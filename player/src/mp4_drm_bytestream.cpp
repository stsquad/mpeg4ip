/**	\file mp4_drm_bytestream.cpp

  	\author Danijel Kopcinovic (danijel.kopcinovic@adnecto.net)
*/

#include "mpeg4ip.h"
#include "mp4_drm_bytestream.h"
#include "player_util.h"

#include "mp4util.h"
#include "mp4array.h"
#include "mp4common.h"

#include "mpeg4ip_sdl_includes.h"
#include "our_config_file.h"

/* disabled warning
	  'identifier' : identifier was truncated to 'number' characters in
	  the debug information
*/
#pragma warning(disable: 4786)

#include <string>
#include "IXMLDocument.h"
#include "IDRMDecryptor.h"
#include "DRMLogger.h"
#include <map>
#include "XMLFactory.h"
#include "PublicCryptoFactory.h"
#include "ThreadSyncFactory.h"

#include "ISMADRMDecManagerFactory.h"
#include "OMADRMDecManagerFactory.h"
#include "OpenIPMPDRMDecManagerFactory.h"
#include "OMADRMInfo.h"
#include "ISMADRMInfo.h"
#include "OpenIPMPDRMInfo.h"

using namespace DRMPlugin;

/*! \brief Holder for DRM decryptors.

    It is used as a container for decryptors for different crypto/drm methods,
    to prevent constantly calling DRM manager for creating decryptors.
*/
class DecryptorHolder {
public:
  /*! \brief  Constructor.
  */
  DecryptorHolder(): doc(0), xmlDoc(0), drm_mutex(SDL_CreateMutex()), id2Dec() {
    XMLFactory::Initialize();
    PublicCryptoFactory::Initialize();
    ThreadSyncFactory::Initialize();
  }

  /*! \brief  Destructor.
  */
  ~DecryptorHolder() {
    SDL_mutexP(drm_mutex);
    std::map<std::string, IDRMDecryptor*>::iterator iter = id2Dec.begin();
    while (iter != id2Dec.end()) {
      delete iter->second;
      iter++;
    }
    if (doc != 0) {
      delete doc; doc = 0;
    }
    XMLFactory::Terminate();
    PublicCryptoFactory::Terminate();
    ThreadSyncFactory::Terminate();
    SDL_mutexV(drm_mutex);
    SDL_DestroyMutex(drm_mutex);
  }

  /*! \brief  Get XML document.

      If XML document is not yet created, create it.

      \returns  XML document. If 0, failed.
  */
  DRMPlugin::IXMLElement* GetXML() {
    SDL_mutexP(drm_mutex);

    if (xmlDoc != 0) {
      SDL_mutexV(drm_mutex);
      return xmlDoc;
    }

    //  Get from config variables.
    std::string xmlFileName = config.get_config_string(CONFIG_OPENIPMPDRM_XMLFILE);
    const char* sensitive_long = config.get_config_string(CONFIG_OPENIPMPDRM_SENSITIVE);
    std::vector<std::string> sensitive;

    char* sens_tmp = (char*)sensitive_long;
    char* sens_limit;
    while ((sens_limit = strchr(sens_tmp, '+')) != NULL) {
      sensitive.push_back(std::string(sens_tmp, sens_limit));
      sens_tmp = ++sens_limit;
    }
    sensitive.push_back(std::string(sens_tmp));

    if (xmlFileName != "") {
      if ((doc = XMLFactory::DecodeDocumentFromFile(xmlFileName)) == 0) {
        SDL_mutexV(drm_mutex);
        return 0;
      }
      if ((xmlDoc = doc->GetRootElement()) == 0) {
        delete doc; doc = 0;
        SDL_mutexV(drm_mutex);
        return 0;
      }

      for (unsigned int count = 0; count < sensitive.size(); count++) {
        char* slash = strchr(sensitive[count].begin(), '/');
        if (slash == NULL) {
          delete doc; doc = 0; xmlDoc = 0;
          SDL_mutexV(drm_mutex);
          return 0;
        }
        std::string path = std::string(sensitive[count].begin(), slash);
        std::string value = std::string(++slash, sensitive[count].end());
        DRMPlugin::IXMLElement* elem = xmlDoc->GetChildElement(path);
        if (elem == 0) {
          delete doc; doc = 0; xmlDoc = 0;
          SDL_mutexV(drm_mutex);
          return 0;
        }
        if (elem->SetStrValue(value) == false) {
          delete doc; doc = 0; xmlDoc = 0;
          SDL_mutexV(drm_mutex);
          return 0;
        }
      }
    }

    SDL_mutexV(drm_mutex);
    return xmlDoc;
  }

  /*! \brief  Get DRM decryptor using information in scheme information atom.

      DRM decryptor can be 0, if scheme information atom indicates that there
      is no protection.

      \param  sinf        input, scheme information atom.
      \param  decryptor   output, DRM decryptor.
      \param  logger      input-output, message logger.

      \returns  Boolean indicating success or failure.
  */
  bool GetDecryptor(MP4Atom* sinf, IDRMDecryptor** decryptor, DRMLogger& logger) {
    *decryptor = 0;
    if (sinf == 0) {
      return true;
    }

    MP4Atom* frma = sinf->FindChildAtom("frma");
    MP4Atom* imif = sinf->FindChildAtom("imif");
    MP4Atom* schm = sinf->FindChildAtom("schm");
    MP4Atom* schi = sinf->FindChildAtom("schi");

    // Original format atom must be present.
    if (frma == 0) {
      return false;
    }

    if (((schm == 0) && (schi != 0)) || ((schm != 0) && (schi == 0))) {
      // Error. Either both schm and schi are defined, or none.
      return false;
    } else if ((schm != 0) && (schi != 0)) {
      // Scheme type and information follows, indicating special scheme protection.
	    MP4Property* scheme_type = 0;
	    schm->FindProperty("schm.scheme_type", &scheme_type, 0);
	    if (scheme_type == 0) {
        // Error. Check the ctor.
        return false;
	    }
	    MP4Property* scheme_version = 0;
	    schm->FindProperty("schm.scheme_version", &scheme_version, 0);
	    if (scheme_version == 0) {
        // Error. Check the ctor.
        return false;
	    }
      if (((MP4Integer32Property*)scheme_type)->GetValue() == ('o'<<24 | 'd'<<16 | 'k'<<8 | 'm')) {
        // OMA DRM protection scheme.
        if (((MP4Integer32Property*)scheme_version)->GetValue() != 0x0200) {
          // Error.
          return false;
        }
        MP4Atom* odkm = schi->FindChildAtom("odkm");
        if (odkm == 0) {
          // Error.
          return false;
        }
        MP4Atom* ohdr = odkm->FindChildAtom("ohdr");
        if (ohdr == 0) {
          // Error.
          return false;
        }

	      MP4Property* EncryptionMethod = 0;
	      ohdr->FindProperty("ohdr.EncryptionMethod", &EncryptionMethod, 0);
	      if (EncryptionMethod == 0) {
          // Error. Check the ctor.
          return false;
	      }

	      MP4Property* EncryptionPadding = 0;
	      ohdr->FindProperty("ohdr.EncryptionPadding", &EncryptionPadding, 0);
	      if (EncryptionPadding == 0) {
          // Error. Check the ctor.
          return false;
	      }

	      MP4Property* PlaintextLength = 0;
	      ohdr->FindProperty("ohdr.PlaintextLength", &PlaintextLength, 0);
	      if (PlaintextLength == 0) {
          // Error. Check the ctor.
          return false;
	      }

	      MP4Property* ContentID = 0;
	      ohdr->FindProperty("ohdr.ContentID", &ContentID, 0);
	      if (ContentID == 0) {
          // Error. Check the ctor.
          return false;
	      }

	      MP4Property* RightsIssuerURL = 0;
	      ohdr->FindProperty("ohdr.RightsIssuerURL", &RightsIssuerURL, 0);
	      if (RightsIssuerURL == 0) {
          // Error. Check the ctor.
          return false;
	      }

	      MP4Property* TextualHeaders = 0;
	      ohdr->FindProperty("ohdr.TextualHeaders", &TextualHeaders, 0);
	      if (TextualHeaders == 0) {
          // Error. Check the ctor.
          return false;
	      }

        u_int8_t* TextualHeadersData;
        u_int32_t tSize;
        ((MP4BytesProperty*)TextualHeaders)->GetValue(&TextualHeadersData, &tSize);

        try {
          OMADRMInfo info(((MP4Integer8Property*)EncryptionMethod)->GetValue(),
            ((MP4Integer8Property*)EncryptionPadding)->GetValue(),
            ((MP4Integer64Property*)PlaintextLength)->GetValue(),
            ((MP4StringProperty*)ContentID)->GetValue(),
            ((MP4Integer32Property*)scheme_version)->GetValue(),
            ((MP4StringProperty*)RightsIssuerURL)->GetValue(),
            (ByteT*)TextualHeadersData, ((MP4BytesProperty*)TextualHeaders)->GetValueSize(0),
            logger);

          MP4Free(TextualHeadersData);

          if (id2Dec.find(info.GetContentID()) != id2Dec.end()) {
            *decryptor = id2Dec[info.GetContentID()];
            return true;
          }

          IOMADRMDecManager* manager =
            OMADRMDecManagerFactory::GetOMADRMDecManager(GetXML(), logger);
          if (manager == 0) {
            return false;
          }

          *decryptor = manager->CreateDecryptor(info, GetXML(), logger);
          delete manager;
          if (*decryptor != 0) {
            id2Dec[info.GetContentID()] = *decryptor;
          }
          return (*decryptor != 0);
        }
        catch (DRMInfoException) {
          MP4Free(TextualHeadersData);
          return false;
        }
      } else if (((MP4Integer32Property*)scheme_type)->GetValue() == ('i'<<24 | 'A'<<16 | 'E'<<8 | 'C')) {
        // ISMA DRM protection scheme.
        if (((MP4Integer32Property*)scheme_version)->GetValue() != 1) {
          // Error.
          return false;
        }
        MP4Atom* iKMS = schi->FindChildAtom("iKMS");
        MP4Atom* iSFM = schi->FindChildAtom("iSFM");
        if ((iKMS == 0) || (iSFM == 0)) {
          // Error.
          return false;
        }

        /*  Currently we don't know how to encode contentID for the ISMA DRM,
            so we put it in the IPMP descriptor (similar to one defined in
            ISMA specifications for streaming).
        */
        if (imif == 0) {
          // Error.
          return false;
        }
	      MP4Property* ipmp_desc = 0;
	      imif->FindProperty("imif.ipmp_desc", &ipmp_desc, 0); 	
	      if (ipmp_desc == 0) {
          // Error. Check the ctor.
          return false;
	      }
        if (((MP4DescriptorProperty*)ipmp_desc)->GetCount() == 0) {
          // No IPMP descriptors.
          return false;
        }
	      MP4Property* IPMPDescriptorId = 0;
	      ((MP4DescriptorProperty*)ipmp_desc)->FindProperty("ipmp_desc.IPMPDescriptorId", &IPMPDescriptorId, 0); 	
	      if (IPMPDescriptorId == 0) {
          // Error. Check the ctor.
          return false;
	      }

	      MP4Property* IPMPSType = 0;
	      ((MP4DescriptorProperty*)ipmp_desc)->FindProperty("ipmp_desc.IPMPSType", &IPMPSType, 0); 	
	      if (IPMPSType == 0) {
          // Error. Check the ctor.
          return false;
	      }

        if (((MP4Integer8Property*)IPMPDescriptorId)->GetValue() != 1) {
          // Error.
          return false;
        }
        if (((MP4Integer16Property*)IPMPSType)->GetValue() != 0x4953) {
          // Error.
          return false;
        }

	      MP4Property* IPMPData = 0;
	      ((MP4DescriptorProperty*)ipmp_desc)->FindProperty("ipmp_desc.IPMPData", &IPMPData, 0); 	
	      if (IPMPData == 0) {
          // Error. Check the ctor.
          return false;
	      }

	      MP4Property* kms_URI = 0;
	      iKMS->FindProperty("iKMS.kms_URI", &kms_URI, 0); 	
	      if (kms_URI == 0) {
          // Error. Check the ctor.
          return false;
	      }

	      MP4Property* selective_encryption = 0;
	      iSFM->FindProperty("iSFM.selective-encryption", &selective_encryption, 0); 	
	      if (selective_encryption == 0) {
          // Error. Check the ctor.
          return false;
	      }

        MP4Property* key_indicator_length = 0;
	      iSFM->FindProperty("iSFM.key-indicator-length", &key_indicator_length, 0); 	
	      if (key_indicator_length == 0) {
          // Error. Check the ctor.
          return false;
	      }

	      MP4Property* IV_length = 0;
	      iSFM->FindProperty("iSFM.IV-length", &IV_length, 0); 	
	      if (IV_length == 0) {
          // Error. Check the ctor.
          return false;
	      }

        u_int8_t* IPMP;
        u_int32_t tSize;
        ((MP4BytesProperty*)IPMPData)->GetValue(&IPMP, &tSize);

        try {
          ISMADRMInfo info((ByteT*)IPMP,
            ((MP4BytesProperty*)IPMPData)->GetValueSize(0),
            ((MP4Integer32Property*)scheme_version)->GetValue(),
            ((MP4StringProperty*)kms_URI)->GetValue(),
            (((MP4BitfieldProperty*)selective_encryption)->GetValue() != 0),
            ((MP4Integer8Property*)key_indicator_length)->GetValue(),
            ((MP4Integer8Property*)IV_length)->GetValue(), logger);

          MP4Free(IPMP);

          if (id2Dec.find(info.GetContentID()) != id2Dec.end()) {
            *decryptor = id2Dec[info.GetContentID()];
            return true;
          }

          IISMADRMDecManager* manager =
            ISMADRMDecManagerFactory::GetISMADRMDecManager(GetXML(), logger);
          if (manager == 0) {
            return false;
          }

          *decryptor = manager->CreateDecryptor(info, GetXML(), logger);
          delete manager;
          if (*decryptor != 0) {
            id2Dec[info.GetContentID()] = *decryptor;
          }
          return (*decryptor != 0);
        }
        catch (DRMInfoException) {
          MP4Free(IPMP);
          return false;
        }
      } else {
        // Unknown protection scheme.
        return false;
      }
    } else if (imif != 0) {
	    MP4Property* ipmp_desc = 0;
	    imif->FindProperty("imif.ipmp_desc", &ipmp_desc, 0); 	
	    if (ipmp_desc == 0) {
        // Error. Check the ctor.
        return false;
	    }
      if (((MP4DescriptorProperty*)ipmp_desc)->GetCount() == 0) {
        // No IPMP descriptors.
        return false;
      }
	    MP4Property* IPMPDescriptorId = 0;
	    ((MP4DescriptorProperty*)ipmp_desc)->FindProperty("ipmp_desc.IPMPDescriptorId", &IPMPDescriptorId, 0); 	
	    if (IPMPDescriptorId == 0) {
        // Error. Check the ctor.
        return false;
	    }

	    MP4Property* IPMPSType = 0;
	    ((MP4DescriptorProperty*)ipmp_desc)->FindProperty("ipmp_desc.IPMPSType", &IPMPSType, 0); 	
	    if (IPMPSType == 0) {
        // Error. Check the ctor.
        return false;
	    }

      if (((MP4Integer8Property*)IPMPDescriptorId)->GetValue() != 255) {
        // Error.
        return false;
      }
      if (((MP4Integer16Property*)IPMPSType)->GetValue() != 65535) {
        // Error.
        return false;
      }

	    MP4Property* IPMPData = 0;
	    ((MP4DescriptorProperty*)ipmp_desc)->FindProperty("ipmp_desc.IPMPData", &IPMPData, 0); 	
	    if (IPMPData == 0) {
        // Error. Check the ctor.
        return false;
	    }

      u_int8_t* IPMP;
      u_int32_t tSize;
      ((MP4BytesProperty*)IPMPData)->GetValue(&IPMP, &tSize);

      //  We recognized "openIPMP" protection.
      try {
        OpenIPMPDRMInfo info((ByteT*)IPMP,
          ((MP4BytesProperty*)IPMPData)->GetValueSize(0), logger);

        MP4Free(IPMP);

        if (id2Dec.find(info.GetContentID()) != id2Dec.end()) {
          *decryptor = id2Dec[info.GetContentID()];
          return true;
        }

        IOpenIPMPDRMDecManager* manager =
          OpenIPMPDRMDecManagerFactory::GetOpenIPMPDRMDecManager(GetXML(), logger);
        if (manager == 0) {
          return false;
        }

        *decryptor = manager->CreateDecryptor(info, GetXML(), logger);
        delete manager;
        if (*decryptor != 0) {
          id2Dec[info.GetContentID()] = *decryptor;
        }
        return (*decryptor != 0);
      }
      catch (DRMInfoException) {
        MP4Free(IPMP);
        return false;
      }
    } else {
      // No protection defined.
      return true;
    }
  }

private:
  SDL_mutex *drm_mutex;
  DRMPlugin::IXMLDocument* doc;
  DRMPlugin::IXMLElement* xmlDoc;
  std::map<std::string, IDRMDecryptor*> id2Dec;
};

/*! \brief  Global decryptor holder.

    Takes care of all DRM related stuff.
*/
static DecryptorHolder holder;

/*! \brief  Get contained sinf atom if exists.

    \param  m_track     input, track identifier.
    \param  m_parent    input, mp4 file.
    \param  pts         input, timestamp.

    \returns  Sinf atom.
*/
static MP4Atom* GetSinfAtom(MP4TrackId m_track, CMp4File *m_parent,
    frame_timestamp_t *pts) {
  MP4Track* track = ((MP4File*)(m_parent->get_file()))->GetTrack(m_track);
  MP4Atom* m_pTrakAtom = track->GetTrakAtom();

	MP4Integer32Property* m_pStscCountProperty;
  MP4Integer32Property* m_pStscSampleDescrIndexProperty;
	MP4Integer32Property* m_pStscFirstSampleProperty;
	if (m_pTrakAtom->FindProperty("trak.mdia.minf.stbl.stsc.entryCount",
      (MP4Property**)&m_pStscCountProperty) == false) {
    /*  This is an error, but we don't handle it here, cause it should be
        handled while parsing mp4 atoms.
    */
    return 0;
  }
	if (m_pTrakAtom->FindProperty("trak.mdia.minf.stbl.stsc.entries.sampleDescriptionIndex",
     (MP4Property**)&m_pStscSampleDescrIndexProperty) == false) {
    /*  This is an error, but we don't handle it here, cause it should be
        handled while parsing mp4 atoms.
    */
    return 0;
  }
	if (m_pTrakAtom->FindProperty("trak.mdia.minf.stbl.stsc.entries.firstSample",
      (MP4Property**)&m_pStscFirstSampleProperty) == false) {
    /*  This is an error, but we don't handle it here, cause it should be
        handled while parsing mp4 atoms.
    */
    return 0;
  }

  MP4SampleId sampleId = track->GetSampleIdFromTime(pts->msec_timestamp);
	u_int32_t stscIndex;
	u_int32_t numStscs = m_pStscCountProperty->GetValue();

	if (numStscs == 0) {
    /*  This is an error, but we don't handle it here, cause it should be
        handled while parsing mp4 atoms.
    */
    return 0;
	}

	for (stscIndex = 0; stscIndex < numStscs; stscIndex++) {
		if (sampleId < m_pStscFirstSampleProperty->GetValue(stscIndex)) {
      if (stscIndex == 0) {
        /*  This is an error, but we don't handle it here, cause it should be
            handled while parsing mp4 atoms.
        */
        return 0;
      }
			stscIndex -= 1;
			break;
		}
	}
	if (stscIndex == numStscs) {
    if (stscIndex == 0) {
      /*  This is an error, but we don't handle it here, cause it should be
          handled while parsing mp4 atoms.
      */
      return 0;
    }
		stscIndex -= 1;
	}

	u_int32_t stsdIndex = m_pStscSampleDescrIndexProperty->GetValue(stscIndex);

	MP4Atom* pStsdAtom = m_pTrakAtom->FindAtom("trak.mdia.minf.stbl.stsd");
  if (pStsdAtom == 0) {
    /*  This is an error, but we don't handle it here, cause it should be
        handled while parsing mp4 atoms.
    */
    return 0;
  }

	MP4Atom* pStsdEntryAtom = pStsdAtom->GetChildAtom(stsdIndex - 1);
  if (pStsdEntryAtom == 0) {
    /*  This is an error, but we don't handle it here, cause it should be
        handled while parsing mp4 atoms.
    */
    return 0;
  }

	return pStsdEntryAtom->FindChildAtom("sinf");
}

/*! Access protected video stream.
*/
/*! \brief  Constructor.
*/
CMp4DRMVideoByteStream::CMp4DRMVideoByteStream(CMp4File *parent, MP4TrackId track):
  CMp4VideoByteStream(parent, track), dBuffer(0), dBuflen(0) {
}

/*! \brief  Destructor.
*/
CMp4DRMVideoByteStream::~CMp4DRMVideoByteStream() {
  if (dBuffer != 0) {
    free(dBuffer); dBuffer = 0;
  }
  dBuflen = 0;
} 

bool CMp4DRMVideoByteStream::start_next_frame (uint8_t **buffer, uint32_t *buflen,
    frame_timestamp_t *pts, void **ud) {
  if (CMp4VideoByteStream::start_next_frame(buffer, buflen, pts, ud) == false) {
    return false;
  }

  if (*buffer != NULL) {
    // Get DRM info.
	  MP4Atom* sinf = GetSinfAtom(m_track, m_parent, pts);

    //  Decrypt sample.
    if (sinf != 0) {
      try {
        IDRMDecryptor* decryptor = 0;
        if (holder.GetDecryptor(sinf, &decryptor, DRMLogger()) == false) {
          *buffer = NULL; *buflen = 0;
          return false;
        }
        if (decryptor != 0) {
          if (dBuffer != 0) {
            free(dBuffer); dBuffer = 0;
          }
          dBuflen = 0;
          if (decryptor->Decrypt((ByteT*)*buffer, *buflen, (ByteT**)&dBuffer,
              &dBuflen, DRMLogger()) == false) {
            *buffer = NULL; *buflen = 0;
            return false;
          }
          *buffer = dBuffer; *buflen = dBuflen;
        }
      }
      catch (...) {
        *buffer = NULL; *buflen = 0;
        return false;
      }
    }
  }

  return true;
}

/*! Access protected audio stream.
*/
/*! \brief  Constructor.
*/
CMp4DRMAudioByteStream::CMp4DRMAudioByteStream(CMp4File *parent, MP4TrackId track):
  CMp4AudioByteStream(parent, track), dBuffer(0), dBuflen(0) {
}

/*! \brief  Destructor.
*/
CMp4DRMAudioByteStream::~CMp4DRMAudioByteStream() {
  if (dBuffer != 0) {
    free(dBuffer); dBuffer = 0;
  }
  dBuflen = 0;
}

bool CMp4DRMAudioByteStream::start_next_frame (uint8_t **buffer, uint32_t *buflen,
    frame_timestamp_t *pts, void **ud) {
  if (CMp4AudioByteStream::start_next_frame(buffer, buflen, pts, ud) == false) {
    return false;
  }

  if (*buffer != NULL) {
    // Get DRM info.
	  MP4Atom* sinf = GetSinfAtom(m_track, m_parent, pts);

    //  Decrypt sample.
    if (sinf != 0) {
      try {
        IDRMDecryptor* decryptor = 0;
        if (holder.GetDecryptor(sinf, &decryptor, DRMLogger()) == false) {
          *buffer = NULL; *buflen = 0;
          return false;
        }
        if (decryptor != 0) {
          if (dBuffer != 0) {
            free(dBuffer); dBuffer = 0;
          }
          dBuflen = 0;
          if (decryptor->Decrypt((ByteT*)*buffer, *buflen, (ByteT**)&dBuffer,
              &dBuflen, DRMLogger()) == false) {
            *buffer = NULL; *buflen = 0;
            return false;
          }
          *buffer = dBuffer; *buflen = dBuflen;
        }
      }
      catch (...) {
        *buffer = NULL; *buflen = 0;
        return false;
      }
    }
  }

  return true;
}
