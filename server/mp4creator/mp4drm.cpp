/**	\file mp4drm.cpp

    \ingroup mpeg4ipencoder

	Example of use of DRM plugins in encoding case.

    The Initial Developer of the Original Code is Adnecto d.o.o.<br>
    Portions created by Adnecto d.o.o. are<br>
    Copyright (C) Adnecto d.o.o. 2005.  All Rights Reserved.<p>

	\author Danijel Kopcinovic (danijel.kopcinovic@adnecto.net)
*/

/*****************************************************************************/
/*  Added by Danijel Kopcinovic as support for DRM encryption.           */
/*****************************************************************************/

#include "mp4creator.h"
#include "mpeg4ip_getopt.h"
#include "mpeg.h"
#include "mp4common.h"

#include "ContentInfoManagerFactory.h"
#include "ISMADRMEncManagerFactory.h"
#include "OMADRMEncManagerFactory.h"
#include "OpenIPMPDRMEncManagerFactory.h"

#include "IDRMEncManager.h"

#include "MPEG4DRMPluginException.h"
#include "OpenIPMP.h"

#include "ICipher.h"
#include "XMLFactory.h"
#include "PublicCryptoFactory.h"
#include "ThreadSyncFactory.h"

#include <string>
#include <vector>

using namespace DRMPlugin;

/*! \brief Encrypts MP4 data with Sinf signalling.

    It is used to encrypt track samples, and add neccessary information
    for decryption.
*/
class IMPEG4SinfDRMEncryptor {
public:
  virtual ~IMPEG4SinfDRMEncryptor() {};

  /*! \brief  Encrypt sample sampleBuffer.
  
      Returns encrypted sample in encSampleData.

      \param    sampleBuffer      input, sample to be encrypted.
      \param    sampleSize        input, size of input sample.
      \param    encSampleData     output, encrypted sample.
      \param    encSampleLen      output, size of encrypted sample.
      \param    logger            input-output, message logger.

      \return Boolean indicating success or failure. In case of failure,
              logger's log will contain error message.
  */
  virtual bool EncryptSample(ByteT* sampleBuffer, UInt32T sampleSize,
    ByteT** encSampleData, UInt32T* encSampleLen, DRMLogger& logger) = 0;

  /*! \brief  Add drm information into sinf atom.
  
      Caller must take care that given sinf atom really coresponds to
      sample description atom which refers to encrypted samples.

      \param    originalFormat  input, 4CC code of the original data format.
      \param    sinf            input-output, sinf atom where to add drm information.
      \param    logger          input-output, message logger.

      \return Boolean indicating success or failure. In case of failure,
              logger's log will contain error message.
  */
  virtual bool Finish(UInt32T originalFormat, MP4Atom* sinf, DRMLogger& logger) = 0;
};

/*! \brief  Encrypts MP4 data using AES128ICM encryptor according to ISMA
            specifications with Sinf signalling.

    Encrypts samples and adds information for decryption to MP4 stream.
*/
class AES128ICMISMAMPEG4SinfDRMEncryptor: public IMPEG4SinfDRMEncryptor {
public:
  /*! \brief  Constructor.

      Mandatory tags in the XML file are:
       - ROOT.RightsHostInfo

      Here follows an example of a correctly formatted XML document.

      \verbatim

      <ROOT>
       <RightsHostInfo>localhost:8080</RightsHostInfo>
      </ROOT>

      \endverbatim

      \param      drm             input, DRM manager.
      \param      conID           input-output, content identifier.
      \param      xmlDoc          input, XML document.
      \param      logger          input-output, message logger.

      If constructor fails, it throws MPEG4DRMPluginException.
  */
  AES128ICMISMAMPEG4SinfDRMEncryptor(IDRMEncManager* drm, std::string& conID,
      DRMPlugin::IXMLElement* xmlDoc, DRMLogger& logger): encryptor(0), contentID(),
      rightsHost() {
    if (xmlDoc == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::ctor: zero XML file.");
      throw MPEG4DRMPluginException();
    }
    encryptor = drm->CreateAES128ICMEnc(conID, xmlDoc, logger);
    if (encryptor == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::ctor: zero encryptor.");
      throw MPEG4DRMPluginException();
    }
    contentID = conID;
    try {
      rightsHost = xmlDoc->GetChildStrValue("RightsHostInfo");
    }
    catch (XMLException) {
      delete encryptor;
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::ctor: missing data in XML file.");
      throw MPEG4DRMPluginException();
    }
  }

  virtual ~AES128ICMISMAMPEG4SinfDRMEncryptor() {
    if (encryptor != 0) delete encryptor;
  }

  /*! \brief  Encrypt sample sampleBuffer.
  
      Returns encrypted sample in encSampleData.

      \param    sampleBuffer      input, sample to be encrypted.
      \param    sampleSize        input, size of input sample.
      \param    encSampleData     output, encrypted sample.
      \param    encSampleLen      output, size of encrypted sample.
      \param    logger            input-output, message logger.

      \return Boolean indicating success or failure. In case of failure,
              logger's log will contain error message.
  */
  virtual bool EncryptSample(ByteT* sampleBuffer, UInt32T sampleSize,
      ByteT** encSampleData, UInt32T* encSampleLen, DRMLogger& logger) {
    int headerLength;

    ByteT* iv;
    UInt32T ivLen;
    if (encryptor->NextIV(&iv, &ivLen, logger) == false) {
      return false;
    }
    //! Sanity check for iv length.
    if (ivLen != 16) {
      if (iv != NULL) free(iv);
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::EncryptSample: invalid initialization vector size.");
      return false;
    }

    ByteT* tmp;
    UInt32T tmpLen;
    if (encryptor->Encrypt(sampleBuffer, sampleSize, &tmp, &tmpLen, logger) == false) {
      if (iv != NULL) free(iv);
      return false;
    }

    headerLength = 0 + ivLen;

    *encSampleData = (ByteT *)malloc(headerLength + tmpLen);
    if (*encSampleData == NULL) {
      if (tmp != NULL) free(tmp); 
      if (iv != NULL) free(iv);
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::EncryptSample: memory allocation error.");
      return false; 
    }
    memcpy((*encSampleData) + headerLength, tmp, tmpLen);
    memset(*encSampleData, 0, 0);
    memcpy((*encSampleData) + 0, iv, ivLen);
    *encSampleLen = headerLength + tmpLen;

    if (tmp != NULL) free(tmp); 
    if (iv != NULL) free(iv);
  
    return true;
  }

  /*! \brief  Add drm information into sinf atom.
  
      Caller must take care that given sinf atom really coresponds to sample
      description atom which refers to encrypted samples.

      \param    originalFormat  input, 4CC code of the original data format.
      \param    sinf            input-output, sinf atom where to add drm information.
      \param    logger          input-output, message logger.

      \return Boolean indicating success or failure. In case of failure,
              logger's log will contain error message.
  */
  virtual bool Finish(UInt32T originalFormat, MP4Atom* sinf, DRMLogger& logger) {
	  MP4Atom* frma = sinf->FindChildAtom("frma");
    if (frma == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: no original format atom.");
      return false;
    }
	  MP4Property* data_format = 0;
	  frma->FindProperty("frma.data-format", &data_format, 0); 	
	  if (data_format == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: frma->SetDataFormat() error.");
      return false;
	  }
    ((MP4Integer32Property*)data_format)->SetValue(originalFormat);

    MP4Atom* schm = sinf->CreateAtom("schm");
    if (schm == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: could not create scheme type atom.");
      return false;
    }
    sinf->AddChildAtom(schm);
    schm->Generate();
	  MP4Property* scheme_type = 0;
	  schm->FindProperty("schm.scheme_type", &scheme_type, 0); 	
	  if (scheme_type == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: schm->SetSchemeCode() error.");
      return false;
	  }
    ((MP4Integer32Property*)scheme_type)->SetValue(('i' << 24) | ('A' << 16) | ('E' << 8) | 'C');

	  MP4Property* scheme_version = 0;
	  schm->FindProperty("schm.scheme_version", &scheme_version, 0); 	
	  if (scheme_version == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: schm->SetSchemeVersion() error.");
      return false;
	  }
    ((MP4Integer32Property*)scheme_version)->SetValue(1);

    MP4Atom* schi = sinf->CreateAtom("schi");
    if (schi == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: could not create scheme information atom.");
      return false;
    }
    sinf->AddChildAtom(schi);
    schi->Generate();

    MP4Atom* iKMS = schi->CreateAtom("iKMS");
    if (iKMS == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: could not create ISMA drm atoms.");
      return false;
    }
    schi->AddChildAtom(iKMS);
    iKMS->Generate();

    MP4Atom* iSFM = schi->CreateAtom("iSFM");
    if (iSFM == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: could not create ISMA drm atoms.");
      return false;
    }
    schi->AddChildAtom(iSFM);
    iSFM->Generate();

	  MP4Property* kms_URI = 0;
	  iKMS->FindProperty("iKMS.kms_URI", &kms_URI, 0); 	
	  if (kms_URI == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: iKMS->SetKeyManagementSystemURI() error.");
      return false;
	  }
    ((MP4StringProperty*)kms_URI)->SetValue(rightsHost.data());

	  MP4Property* selective_encryption = 0;
	  iSFM->FindProperty("iSFM.selective-encryption", &selective_encryption, 0); 	
	  if (selective_encryption == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: iSFM->SetSelectiveEncryption() error.");
      return false;
	  }
    ((MP4BitfieldProperty*)selective_encryption)->SetValue(0);

	  MP4Property* key_indicator_length = 0;
	  iSFM->FindProperty("iSFM.key-indicator-length", &key_indicator_length, 0); 	
	  if (key_indicator_length == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: iSFM->SetKeyIndicatorLength() error.");
      return false;
	  }
    ((MP4Integer8Property*)key_indicator_length)->SetValue(0);

	  MP4Property* IV_length = 0;
	  iSFM->FindProperty("iSFM.IV-length", &IV_length, 0); 	
	  if (IV_length == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: iSFM->SetIVLength() error.");
      return false;
	  }
    ((MP4Integer8Property*)IV_length)->SetValue(16);

    MP4Atom* imif = sinf->CreateAtom("imif");
    if (imif == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: could not create IPMP info atom.");
      return false;
    }
    sinf->AddChildAtom(imif);
    imif->Generate();

	  MP4Property* ipmp_desc = 0;
	  imif->FindProperty("imif.ipmp_desc", &ipmp_desc, 0); 	
	  if (ipmp_desc == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: missing IPMP descriptor property.");
      return false;
	  }
    MP4Descriptor* ipmpDesc = ((MP4DescriptorProperty*)ipmp_desc)->AddDescriptor(MP4IPMPDescrTag);
    if (ipmpDesc == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: could not create IPMP descriptor.");
      return false;
    }

	  MP4Property* IPMPDescriptorId = 0;
	  ipmpDesc->FindProperty("IPMPDescriptorId", &IPMPDescriptorId, 0); 	
	  if (IPMPDescriptorId == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: ipmpDesc->SetIPMPDescriptorIdentifier() error.");
      return false;
	  }
    ((MP4Integer8Property*)IPMPDescriptorId)->SetValue(1);

	  MP4Property* IPMPSType = 0;
	  ipmpDesc->FindProperty("IPMPSType", &IPMPSType, 0); 	
	  if (IPMPSType == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: ipmpDesc->SetIPMPDescriptorType() error.");
      return false;
	  }
    ((MP4Integer16Property*)IPMPSType)->SetValue(0x4953);

	  MP4Property* IPMPData = 0;
	  ipmpDesc->FindProperty("IPMPData", &IPMPData, 0); 	
	  if (IPMPData == 0) {
      logger.UpdateLog("AES128ICMISMAMPEG4SinfDRMEncryptor::Finish: ipmpDesc->SetIPMPData() error.");
      return false;
	  }
    ((MP4BytesProperty*)IPMPData)->SetValue((u_int8_t*)(contentID.data()), contentID.size() + 1);

    return true;
  }

private:
  AES128ICMEncryptor* encryptor;
  std::string contentID;
  std::string rightsHost;
};

/*! \brief  Encrypts MP4 data using AES128CBC encryptor according to OMA
            specifications with Sinf signalling.

    Encrypts samples and adds information for decryption to MP4 stream.
*/
class AES128CBCOMAMPEG4SinfDRMEncryptor: public IMPEG4SinfDRMEncryptor {
public:
  /*! \brief  Constructor.

      Mandatory tags in the XML file are:
       - ROOT.RightsHostInfo

      Here follows an example of a correctly formatted XML document.

      \verbatim

      <ROOT>
       <RightsHostInfo>localhost:8080</RightsHostInfo>
       <SilentHeader>Silent:on-demand;http://www.silent.com/</SilentHeader>
       <PreviewHeader>Preview:instant;http://www.preview.com/</PreviewHeader>
       <ContentURLHeader>ContentURL:http://www.content.com/</ContentURLHeader>
       <ContentVersionHeader>ContentVersion:original-content-identifier:1.1</ContentVersionHeader>
       <ContentLocationHeader>Content-Location:http://www.content.com/</ContentLocationHeader>
      </ROOT>

      \endverbatim

      \param      drm             input, DRM manager.
      \param      conID           input-output, content identifier.
      \param      xmlDoc          input, XML document.
      \param      logger          input-output, message logger.

      If constructor fails, it throws MPEG4DRMPluginException.
  */
  AES128CBCOMAMPEG4SinfDRMEncryptor(IDRMEncManager* drm, std::string& conID,
      DRMPlugin::IXMLElement* xmlDoc, DRMLogger& logger): encryptor(0), contentID(),
      rightsHost(), silentHdr(), previewHdr(), contentURLHdr(), contentVerHdr(),
      contentLocHdr(), pTextLen(0) {
    if (xmlDoc == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::ctor: zero XML file.");
      throw MPEG4DRMPluginException();
    }
    encryptor = drm->CreateAES128CBCEnc(conID, xmlDoc, logger);
    if (encryptor == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::ctor: zero encryptor.");
      throw MPEG4DRMPluginException();
    }
    contentID = conID;
    try {
      rightsHost = xmlDoc->GetChildStrValue("RightsHostInfo");
    }
    catch (XMLException) {
      delete encryptor;
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::ctor: missing data in XML file.");
      throw MPEG4DRMPluginException();
    }
    try {
      silentHdr = xmlDoc->GetChildStrValue("SilentHeader");
    }
    catch (XMLException) {
    }
    try {
      previewHdr = xmlDoc->GetChildStrValue("PreviewHeader");
    }
    catch (XMLException) {
    }
    try {
      contentURLHdr = xmlDoc->GetChildStrValue("ContentURLHeader");
    }
    catch (XMLException) {
    }
    try {
      contentVerHdr = xmlDoc->GetChildStrValue("ContentVersionHeader");
    }
    catch (XMLException) {
    }
    try {
      contentLocHdr = xmlDoc->GetChildStrValue("ContentLocationHeader");
    }
    catch (XMLException) {
    }
  }


  virtual ~AES128CBCOMAMPEG4SinfDRMEncryptor() {
    if (encryptor != 0) delete encryptor;
  }

  /*! \brief  Encrypt sample sampleBuffer.
  
      Returns encrypted sample in encSampleData.

      \param    sampleBuffer      input, sample to be encrypted.
      \param    sampleSize        input, size of input sample.
      \param    encSampleData     output, encrypted sample.
      \param    encSampleLen      output, size of encrypted sample.
      \param    logger            input-output, message logger.

      \return Boolean indicating success or failure. In case of failure,
              logger's log will contain error message.
  */
  virtual bool EncryptSample(ByteT* sampleBuffer, UInt32T sampleSize,
      ByteT** encSampleData, UInt32T* encSampleLen, DRMLogger& logger) {
    //  Get next IV.
    ByteT* iv;
    UInt32T ivLen;
    if (encryptor->NextIV(&iv, &ivLen, logger) == false) {
      return false;
    }
    //  Encrypt sample.
    ByteT* enc;
    UInt32T encLen;
    //  Encrypt sample.
    if (encryptor->Encrypt(sampleBuffer, sampleSize, &enc, &encLen, logger) == false) {
      if (iv != NULL) {
        free(iv);
      }
      return false;
    }

    /*  Allocate memory (1 byte for selective encryption + 1 for iv length +
        1 for counter length + size of the IV + size of the encrypted block).
    */
    *encSampleData = (ByteT*)malloc(3 + ivLen + encLen);
    if (*encSampleData == NULL) {
      if (iv != NULL) {
        free(iv);
      }
      if (enc != NULL) {
        free(enc);
      }
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::EncryptSample: memory allocation error.");
      return false;
    }
    *encSampleLen = 3 + ivLen + encLen;
    ByteT* tmp = *encSampleData;
    //  Add AU header.
    //  Selective encryption indicator.
  //  *tmp++ = (contentInfo.GetSelectiveEncryption() == true? 0x80: 0);
    *tmp++ = 0x80;
    //  Counter length.
    *tmp++ = 0;
    //  IV length.
    *tmp++ = (ByteT)ivLen;
    memcpy(tmp, iv, ivLen);
    tmp += ivLen;
    memcpy(tmp, enc, encLen);
    if (iv != NULL) {
      free(iv);
    }
    if (enc != NULL) {
      free(enc);
    }
    pTextLen += sampleSize;

    return true;
  }

  /*! \brief  Add drm information into sinf atom.
  
      Caller must take care that given sinf atom really coresponds to sample 
      description atom which refers to encrypted samples.

      \param    originalFormat  input, 4CC code of the original data format.
      \param    sinf            input-output, sinf atom where to add drm information.
      \param    logger          input-output, message logger.

      \return Boolean indicating success or failure. In case of failure,
              logger's log will contain error message.
  */
  virtual bool Finish(UInt32T originalFormat, MP4Atom* sinf, DRMLogger& logger) {
	  MP4Atom* frma = sinf->FindChildAtom("frma");
    if (frma == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: no original format atom.");
      return false;
    }
	  MP4Property* data_format = 0;
	  frma->FindProperty("frma.data-format", &data_format, 0); 	
	  if (data_format == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: frma->SetDataFormat() error.");
      return false;
	  }
    ((MP4Integer32Property*)data_format)->SetValue(originalFormat);

    MP4Atom* schm = sinf->CreateAtom("schm");
    if (schm == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: could not create scheme type atom.");
      return false;
    }
    sinf->AddChildAtom(schm);
    schm->Generate();

	  MP4Property* scheme_type = 0;
	  schm->FindProperty("schm.scheme_type", &scheme_type, 0); 	
	  if (scheme_type == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: schm->SetSchemeCode() error.");
      return false;
	  }
    ((MP4Integer32Property*)scheme_type)->SetValue(('o' << 24) | ('d' << 16) | ('k' << 8) | 'm');

	  MP4Property* scheme_version = 0;
	  schm->FindProperty("schm.scheme_version", &scheme_version, 0); 	
	  if (scheme_version == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: schm->SetSchemeVersion() error.");
      return false;
	  }
    ((MP4Integer32Property*)scheme_version)->SetValue(0x0200);

    MP4Atom* schi = sinf->CreateAtom("schi");
    if (schi == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: could not create scheme information atom.");
      return false;
    }
    sinf->AddChildAtom(schi);
    schi->Generate();

    MP4Atom* odkm = schi->CreateAtom("odkm");
    if (odkm == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: could not create OMA drm atom.");
      return false;
    }
    schi->AddChildAtom(odkm);
    odkm->Generate();

    MP4Atom* ohdr = odkm->FindChildAtom("ohdr");
    if (ohdr == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: could not create OMA drm headers atom.");
      return false;
    }

	  MP4Property* version = 0;
	  ohdr->FindProperty("ohdr.version", &version, 0); 	
	  if (version == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetAtomVersion() error.");
      return false;
	  }
    ((MP4Integer8Property*)version)->SetValue(0);

	  MP4Property* flags = 0;
	  ohdr->FindProperty("ohdr.flags", &flags, 0); 	
	  if (flags == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetAtomFlags() error.");
      return false;
	  }
    ((MP4Integer24Property*)flags)->SetValue(0);

	  MP4Property* EncryptionMethod = 0;
	  ohdr->FindProperty("ohdr.EncryptionMethod", &EncryptionMethod, 0); 	
	  if (EncryptionMethod == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetEncryptionMethod() error.");
      return false;
	  }
    ((MP4Integer8Property*)EncryptionMethod)->SetValue(1);

	  MP4Property* EncryptionPadding = 0;
	  ohdr->FindProperty("ohdr.EncryptionPadding", &EncryptionPadding, 0); 	
	  if (EncryptionPadding == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetEncryptionPadding() error.");
      return false;
	  }
    ((MP4Integer8Property*)EncryptionPadding)->SetValue(1);

	  MP4Property* PlaintextLength = 0;
	  ohdr->FindProperty("ohdr.PlaintextLength", &PlaintextLength, 0); 	
	  if (PlaintextLength == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetPlaintextLength() error.");
      return false;
	  }
    ((MP4Integer64Property*)PlaintextLength)->SetValue(pTextLen);

	  MP4Property* ContentID = 0;
	  ohdr->FindProperty("ohdr.ContentID", &ContentID, 0); 	
	  if (ContentID == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetContentIdentifier() error.");
      return false;
	  }
    ((MP4StringProperty*)ContentID)->SetFixedLength(strlen(contentID.data()));
    ((MP4StringProperty*)ContentID)->SetValue(contentID.data());

	  MP4Property* ContentIDLength = 0;
	  ohdr->FindProperty("ohdr.ContentIDLength", &ContentIDLength, 0);
	  if (ContentIDLength == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetContentIdentifier() error.");
      return false;
	  }
    ((MP4Integer16Property*)ContentIDLength)->SetValue(strlen(contentID.data()));

	  MP4Property* RightsIssuerURL = 0;
	  ohdr->FindProperty("ohdr.RightsIssuerURL", &RightsIssuerURL, 0); 	
	  if (RightsIssuerURL == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetRightsIssuerURL() error.");
      return false;
	  }
    ((MP4StringProperty*)RightsIssuerURL)->SetFixedLength(strlen(rightsHost.data()));
    ((MP4StringProperty*)RightsIssuerURL)->SetValue(rightsHost.data());
	  MP4Property* RightsIssuerURLLength = 0;
	  ohdr->FindProperty("ohdr.RightsIssuerURLLength", &RightsIssuerURLLength, 0); 	
	  if (RightsIssuerURLLength == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetRightsIssuerURL() error.");
      return false;
	  }
    ((MP4Integer16Property*)RightsIssuerURLLength)->SetValue(strlen(rightsHost.data()));

    unsigned int len = 0;
    unsigned int tmpLen;
    tmpLen = silentHdr.size();
    len += (tmpLen > 0? tmpLen + 1: 0);
    tmpLen = previewHdr.size();
    len += (tmpLen > 0? tmpLen + 1: 0);
    tmpLen = contentURLHdr.size();
    len += (tmpLen > 0? tmpLen + 1: 0);
    tmpLen = contentVerHdr.size();
    len += (tmpLen > 0? tmpLen + 1: 0);
    tmpLen = contentLocHdr.size();
    len += (tmpLen > 0? tmpLen + 1: 0);

    ByteT* pValue = (ByteT*)malloc(len);
    if (pValue == NULL) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: memory allocation error.");
      return false;
    }
    unsigned int pos = 0;
    tmpLen = silentHdr.size();
    if (tmpLen > 0) {
      memcpy(pValue + pos, silentHdr.data(), tmpLen + 1);
      pos += (tmpLen + 1);
    }
    tmpLen = previewHdr.size();
    if (tmpLen > 0) {
      memcpy(pValue + pos, previewHdr.data(), tmpLen + 1);
      pos += (tmpLen + 1);
    }
    tmpLen = contentURLHdr.size();
    if (tmpLen > 0) {
      memcpy(pValue + pos, contentURLHdr.data(), tmpLen + 1);
      pos += (tmpLen + 1);
    }
    tmpLen = contentVerHdr.size();
    if (tmpLen > 0) {
      memcpy(pValue + pos, contentVerHdr.data(), tmpLen + 1);
      pos += (tmpLen + 1);
    }
    tmpLen = contentLocHdr.size();
    if (tmpLen > 0) {
      memcpy(pValue + pos, contentLocHdr.data(), tmpLen + 1);
      pos += (tmpLen + 1);
    }

	  MP4Property* TextualHeaders = 0;
	  ohdr->FindProperty("ohdr.TextualHeaders", &TextualHeaders, 0); 	
	  if (TextualHeaders == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetTextualHeaders() error.");
      return false;
	  }
    ((MP4BytesProperty*)TextualHeaders)->SetValue((u_int8_t*)pValue, len);
	  MP4Property* TextualHeadersLength = 0;
	  ohdr->FindProperty("ohdr.TextualHeadersLength", &TextualHeadersLength, 0); 	
	  if (TextualHeadersLength == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetTextualHeaders() error.");
      return false;
	  }
    ((MP4Integer16Property*)TextualHeadersLength)->SetValue(len);

    if (pValue != 0) {
      free(pValue);
    }

	  odkm->FindProperty("odkm.version", &version, 0); 	
	  if (version == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: odkm->SetAtomVersion() error.");
      return false;
	  }
    ((MP4Integer8Property*)version)->SetValue(0);

	  odkm->FindProperty("odkm.flags", &flags, 0); 	
	  if (flags == 0) {
      logger.UpdateLog("AES128CBCOMAMPEG4SinfDRMEncryptor::Finish: odkm->SetAtomFlags() error.");
      return false;
	  }
    ((MP4Integer24Property*)flags)->SetValue(0);

    return true;
  }

private:
  AES128CBCEncryptor* encryptor;
  std::string contentID;
  std::string rightsHost;
  std::string silentHdr;
  std::string previewHdr;
  std::string contentURLHdr;
  std::string contentVerHdr;
  std::string contentLocHdr;
  UInt64T pTextLen;
};

/*! \brief  Encrypts MP4 data using AES128CTR encryptor according to OMA
            specifications with Sinf signalling.

    Encrypts samples and adds information for decryption to MP4 stream.
*/
class AES128CTROMAMPEG4SinfDRMEncryptor: public IMPEG4SinfDRMEncryptor {
public:
  /*! \brief  Constructor.

      Mandatory tags in the XML file are:
       - ROOT.RightsHostInfo

      Here follows an example of a correctly formatted XML document.

      \verbatim

      <ROOT>
       <RightsHostInfo>localhost:8080</RightsHostInfo>
       <SilentHeader>Silent:on-demand;http://www.silent.com/</SilentHeader>
       <PreviewHeader>Preview:instant;http://www.preview.com/</PreviewHeader>
       <ContentURLHeader>ContentURL:http://www.content.com/</ContentURLHeader>
       <ContentVersionHeader>ContentVersion:original-content-identifier:1.1</ContentVersionHeader>
       <ContentLocationHeader>Content-Location:http://www.content.com/</ContentLocationHeader>
      </ROOT>

      \endverbatim

      \param      drm             input, DRM manager.
      \param      conID           input, content identifier.
      \param      xmlDoc          input, XML document.
      \param      logger          input-output, message logger.

      If constructor fails, it throws MPEG4DRMPluginException.
  */
  AES128CTROMAMPEG4SinfDRMEncryptor(IDRMEncManager* drm, std::string& conID,
      DRMPlugin::IXMLElement* xmlDoc, DRMLogger& logger): encryptor(0), contentID(),
      rightsHost(), silentHdr(), previewHdr(), contentURLHdr(), contentVerHdr(),
      contentLocHdr(), pTextLen(0) {
    if (xmlDoc == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::ctor: zero XML file.");
      throw MPEG4DRMPluginException();
    }
    encryptor = drm->CreateAES128CTREnc(conID, xmlDoc, logger);
    if (encryptor == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::ctor: zero encryptor.");
      throw MPEG4DRMPluginException();
    }
    contentID = conID;
    try {
      rightsHost = xmlDoc->GetChildStrValue("RightsHostInfo");
    }
    catch (XMLException) {
      delete encryptor;
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::ctor: missing data in XML file.");
      throw MPEG4DRMPluginException();
    }
    try {
      silentHdr = xmlDoc->GetChildStrValue("SilentHeader");
    }
    catch (XMLException) {
    }
    try {
      previewHdr = xmlDoc->GetChildStrValue("PreviewHeader");
    }
    catch (XMLException) {
    }
    try {
      contentURLHdr = xmlDoc->GetChildStrValue("ContentURLHeader");
    }
    catch (XMLException) {
    }
    try {
      contentVerHdr = xmlDoc->GetChildStrValue("ContentVersionHeader");
    }
    catch (XMLException) {
    }
    try {
      contentLocHdr = xmlDoc->GetChildStrValue("ContentLocationHeader");
    }
    catch (XMLException) {
    }
  }

  virtual ~AES128CTROMAMPEG4SinfDRMEncryptor() {
    if (encryptor != 0) delete encryptor;
  }

  /*! \brief  Encrypt sample sampleBuffer.
  
      Returns encrypted sample in encSampleData.

      \param    sampleBuffer      input, sample to be encrypted.
      \param    sampleSize        input, size of input sample.
      \param    encSampleData     output, encrypted sample.
      \param    encSampleLen      output, size of encrypted sample.
      \param    logger            input-output, message logger.

      \return Boolean indicating success or failure. In case of failure,
              logger's log will contain error message.
  */
  virtual bool EncryptSample(ByteT* sampleBuffer, UInt32T sampleSize,
      ByteT** encSampleData, UInt32T* encSampleLen, DRMLogger& logger) {
    //  Get next counter.
    ByteT* ctr;
    UInt32T ctrLen;
    if (encryptor->NextCtr(&ctr, &ctrLen, logger) == false) {
      return false;
    }
    //  Encrypt sample.
    ByteT* enc;
    UInt32T encLen;
    //  Encrypt sample.
    if (encryptor->Encrypt(sampleBuffer, sampleSize, &enc, &encLen, logger) == false) {
      if (ctr != NULL) {
        free(ctr);
      }
      return false;
    }

    /*  Allocate memory (1 byte for selective encryption + 1 for iv length +
        1 for counter length + size of the counter + size of the encrypted block).
    */
    *encSampleData = (ByteT*)malloc(3 + ctrLen + encLen);
    if (*encSampleData == NULL) {
      if (ctr != NULL) {
        free(ctr);
      }
      if (enc != NULL) {
        free(enc);
      }
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::EncryptSample: memory allocation error.");
      return false;
    }
    *encSampleLen = 3 + ctrLen + encLen;
    ByteT* tmp = *encSampleData;
    //  Add AU header.
    //  Selective encryption indicator.
  //  *tmp++ = (contentInfo.GetSelectiveEncryption() == true? 0x80: 0);
    *tmp++ = 0x80;
    //  Counter length.
    *tmp++ = (ByteT)ctrLen;
    //  IV length.
    *tmp++ = 0;
    memcpy(tmp, ctr, ctrLen);
    tmp += ctrLen;
    memcpy(tmp, enc, encLen);
    if (ctr != NULL) {
      free(ctr);
    }
    if (enc != NULL) {
      free(enc);
    }
    pTextLen += sampleSize;

    return true;
  }

  /*! \brief  Add drm information into sinf atom.
  
      Caller must take care that given sinf atom really coresponds to sample
      description atom which refers to encrypted samples.

      \param    originalFormat  input, 4CC code of the original data format.
      \param    sinf            input-output, sinf atom where to add drm information.
      \param    logger          input-output, message logger.

      \return Boolean indicating success or failure. In case of failure,
              logger's log will contain error message.
  */
  virtual bool Finish(UInt32T originalFormat, MP4Atom* sinf, DRMLogger& logger) {
	  MP4Atom* frma = sinf->FindChildAtom("frma");
    if (frma == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: no original format atom.");
      return false;
    }
	  MP4Property* data_format = 0;
	  frma->FindProperty("frma.data-format", &data_format, 0); 	
	  if (data_format == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: frma->SetDataFormat() error.");
      return false;
	  }
    ((MP4Integer32Property*)data_format)->SetValue(originalFormat);

    MP4Atom* schm = sinf->CreateAtom("schm");
    if (schm == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: could not create scheme type atom.");
      return false;
    }
    sinf->AddChildAtom(schm);
    schm->Generate();

	  MP4Property* scheme_type = 0;
	  schm->FindProperty("schm.scheme_type", &scheme_type, 0); 	
	  if (scheme_type == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: schm->SetSchemeCode() error.");
      return false;
	  }
    ((MP4Integer32Property*)scheme_type)->SetValue(('o' << 24) | ('d' << 16) | ('k' << 8) | 'm');

	  MP4Property* scheme_version = 0;
	  schm->FindProperty("schm.scheme_version", &scheme_version, 0); 	
	  if (scheme_version == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: schm->SetSchemeVersion() error.");
      return false;
	  }
    ((MP4Integer32Property*)scheme_version)->SetValue(0x0200);

    MP4Atom* schi = sinf->CreateAtom("schi");
    if (schi == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: could not create scheme information atom.");
      return false;
    }
    sinf->AddChildAtom(schi);
    schi->Generate();

    MP4Atom* odkm = schi->CreateAtom("odkm");
    if (odkm == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: could not create OMA drm atom.");
      return false;
    }
    schi->AddChildAtom(odkm);
    odkm->Generate();

    MP4Atom* ohdr = odkm->FindChildAtom("ohdr");
    if (ohdr == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: could not create OMA drm headers atom.");
      return false;
    }

	  MP4Property* version = 0;
	  ohdr->FindProperty("ohdr.version", &version, 0); 	
	  if (version == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetAtomVersion() error.");
      return false;
	  }
    ((MP4Integer8Property*)version)->SetValue(0);

	  MP4Property* flags = 0;
	  ohdr->FindProperty("ohdr.flags", &flags, 0); 	
	  if (flags == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetAtomFlags() error.");
      return false;
	  }
    ((MP4Integer24Property*)flags)->SetValue(0);

	  MP4Property* EncryptionMethod = 0;
	  ohdr->FindProperty("ohdr.EncryptionMethod", &EncryptionMethod, 0); 	
	  if (EncryptionMethod == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetEncryptionMethod() error.");
      return false;
	  }
    ((MP4Integer8Property*)EncryptionMethod)->SetValue(2);

	  MP4Property* EncryptionPadding = 0;
	  ohdr->FindProperty("ohdr.EncryptionPadding", &EncryptionPadding, 0); 	
	  if (EncryptionPadding == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetEncryptionPadding() error.");
      return false;
	  }
    ((MP4Integer8Property*)EncryptionPadding)->SetValue(0);

	  MP4Property* PlaintextLength = 0;
	  ohdr->FindProperty("ohdr.PlaintextLength", &PlaintextLength, 0); 	
	  if (PlaintextLength == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetPlaintextLength() error.");
      return false;
	  }
    ((MP4Integer64Property*)PlaintextLength)->SetValue(pTextLen);

	  MP4Property* ContentID = 0;
	  ohdr->FindProperty("ohdr.ContentID", &ContentID, 0); 	
	  if (ContentID == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetContentIdentifier() error.");
      return false;
	  }
    ((MP4StringProperty*)ContentID)->SetFixedLength(strlen(contentID.data()));
    ((MP4StringProperty*)ContentID)->SetValue(contentID.data());

	  MP4Property* ContentIDLength = 0;
	  ohdr->FindProperty("ohdr.ContentIDLength", &ContentIDLength, 0);
	  if (ContentIDLength == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetContentIdentifier() error.");
      return false;
	  }
    ((MP4Integer16Property*)ContentIDLength)->SetValue(strlen(contentID.data()));

	  MP4Property* RightsIssuerURL = 0;
	  ohdr->FindProperty("ohdr.RightsIssuerURL", &RightsIssuerURL, 0); 	
	  if (RightsIssuerURL == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetRightsIssuerURL() error.");
      return false;
	  }
    ((MP4StringProperty*)RightsIssuerURL)->SetFixedLength(strlen(rightsHost.data()));
    ((MP4StringProperty*)RightsIssuerURL)->SetValue(rightsHost.data());
	  MP4Property* RightsIssuerURLLength = 0;
	  ohdr->FindProperty("ohdr.RightsIssuerURLLength", &RightsIssuerURLLength, 0); 	
	  if (RightsIssuerURLLength == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetRightsIssuerURL() error.");
      return false;
	  }
    ((MP4Integer16Property*)RightsIssuerURLLength)->SetValue(strlen(rightsHost.data()));

    unsigned int len = 0;
    unsigned int tmpLen;
    tmpLen = silentHdr.size();
    len += (tmpLen > 0? tmpLen + 1: 0);
    tmpLen = previewHdr.size();
    len += (tmpLen > 0? tmpLen + 1: 0);
    tmpLen = contentURLHdr.size();
    len += (tmpLen > 0? tmpLen + 1: 0);
    tmpLen = contentVerHdr.size();
    len += (tmpLen > 0? tmpLen + 1: 0);
    tmpLen = contentLocHdr.size();
    len += (tmpLen > 0? tmpLen + 1: 0);

    ByteT* pValue = (ByteT*)malloc(len);
    if (pValue == NULL) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: memory allocation error.");
      return false;
    }
    unsigned int pos = 0;
    tmpLen = silentHdr.size();
    if (tmpLen > 0) {
      memcpy(pValue + pos, silentHdr.data(), tmpLen + 1);
      pos += (tmpLen + 1);
    }
    tmpLen = previewHdr.size();
    if (tmpLen > 0) {
      memcpy(pValue + pos, previewHdr.data(), tmpLen + 1);
      pos += (tmpLen + 1);
    }
    tmpLen = contentURLHdr.size();
    if (tmpLen > 0) {
      memcpy(pValue + pos, contentURLHdr.data(), tmpLen + 1);
      pos += (tmpLen + 1);
    }
    tmpLen = contentVerHdr.size();
    if (tmpLen > 0) {
      memcpy(pValue + pos, contentVerHdr.data(), tmpLen + 1);
      pos += (tmpLen + 1);
    }
    tmpLen = contentLocHdr.size();
    if (tmpLen > 0) {
      memcpy(pValue + pos, contentLocHdr.data(), tmpLen + 1);
      pos += (tmpLen + 1);
    }

	  MP4Property* TextualHeaders = 0;
	  ohdr->FindProperty("ohdr.TextualHeaders", &TextualHeaders, 0); 	
	  if (TextualHeaders == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetTextualHeaders() error.");
      return false;
	  }
    ((MP4BytesProperty*)TextualHeaders)->SetValue((u_int8_t*)pValue, len);
	  MP4Property* TextualHeadersLength = 0;
	  ohdr->FindProperty("ohdr.TextualHeadersLength", &TextualHeadersLength, 0); 	
	  if (TextualHeadersLength == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: ohdr->SetTextualHeaders() error.");
      return false;
	  }
    ((MP4Integer16Property*)TextualHeadersLength)->SetValue(len);

    if (pValue != 0) {
      free(pValue);
    }

	  odkm->FindProperty("odkm.version", &version, 0); 	
	  if (version == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: odkm->SetAtomVersion() error.");
      return false;
	  }
    ((MP4Integer8Property*)version)->SetValue(0);

	  odkm->FindProperty("odkm.flags", &flags, 0); 	
	  if (flags == 0) {
      logger.UpdateLog("AES128CTROMAMPEG4SinfDRMEncryptor::Finish: odkm->SetAtomFlags() error.");
      return false;
	  }
    ((MP4Integer24Property*)flags)->SetValue(0);

    return true;
  }

private:
  AES128CTREncryptor* encryptor;
  std::string contentID;
  std::string rightsHost;
  std::string silentHdr;
  std::string previewHdr;
  std::string contentURLHdr;
  std::string contentVerHdr;
  std::string contentLocHdr;
  UInt64T pTextLen;
};

/*! \brief  Encrypts MP4 data using AES128CTR encryptor according to openIPMP
            specifications with Sinf signalling.

    Encrypts samples and adds information for decryption to MP4 stream.
*/
class AES128CTROpenIPMPMPEG4SinfDRMEncryptor: public IMPEG4SinfDRMEncryptor {
public:
  /*! \brief  Constructor.

      Mandatory tags in the XML file are:
       - ROOT.RightsHostInfo

      Here follows an example of a correctly formatted XML document.

      \verbatim

      <ROOT>
       <RightsHostInfo>localhost:8080</RightsHostInfo>
      </ROOT>

      \endverbatim

      \param      drm             input, DRM manager.
      \param      conID           input, content identifier.
      \param      xmlDoc          input, XML document.
      \param      logger          input-output, message logger.

      If constructor fails, it throws MPEG4DRMPluginException.
  */
  AES128CTROpenIPMPMPEG4SinfDRMEncryptor(IDRMEncManager* drm, std::string& conID,
      DRMPlugin::IXMLElement* xmlDoc, DRMLogger& logger): encryptor(0), contentID(),
      rightsHost() {
    if (xmlDoc == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::ctor: zero XML file.");
      throw MPEG4DRMPluginException();
    }
    encryptor = drm->CreateAES128CTREnc(conID, xmlDoc, logger);
    if (encryptor == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::ctor: zero encryptor.");
      throw MPEG4DRMPluginException();
    }
    contentID = conID;
    try {
      rightsHost = xmlDoc->GetChildStrValue("RightsHostInfo");
    }
    catch (XMLException) {
      delete encryptor;
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::ctor: missing data in XML file.");
      throw MPEG4DRMPluginException();
    }
  }

  virtual ~AES128CTROpenIPMPMPEG4SinfDRMEncryptor() {
    if (encryptor != 0) delete encryptor;
  }

  /*! \brief  Encrypt sample sampleBuffer.
  
      Returns encrypted sample in encSampleData.

      \param    sampleBuffer      input, sample to be encrypted.
      \param    sampleSize        input, size of input sample.
      \param    encSampleData     output, encrypted sample.
      \param    encSampleLen      output, size of encrypted sample.
      \param    logger            input-output, message logger.

      \return Boolean indicating success or failure. In case of failure,
              logger's log will contain error message.
  */
  virtual bool EncryptSample(ByteT* sampleBuffer, UInt32T sampleSize,
      ByteT** encSampleData, UInt32T* encSampleLen, DRMLogger& logger) {
    //  Get next counter.
    ByteT* ctr;
    UInt32T ctrLen;
    if (encryptor->NextCtr(&ctr, &ctrLen, logger) == false) {
      return false;
    }
    //  Encrypt sample.
    ByteT* enc;
    UInt32T encLen;
    //  Encrypt sample.
    if (encryptor->Encrypt(sampleBuffer, sampleSize, &enc, &encLen, logger) == false) {
      if (ctr != NULL) {
        free(ctr);
      }
      return false;
    }

    /*  Allocate memory (1 for counter length + size of the counter + size of the
        encrypted block).
    */
    *encSampleData = (ByteT*)malloc(1 + ctrLen + encLen);
    if (*encSampleData == NULL) {
      if (ctr != NULL) {
        free(ctr);
      }
      if (enc != NULL) {
        free(enc);
      }
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::EncryptSample: memory allocation error.");
      return false;
    }
    *encSampleLen = 1 + ctrLen + encLen;
    ByteT* tmp = *encSampleData;
    //  Add header.
    //  Counter length.
    *tmp++ = (ByteT)ctrLen;
    memcpy(tmp, ctr, ctrLen);
    tmp += ctrLen;
    memcpy(tmp, enc, encLen);
    if (ctr != NULL) {
      free(ctr);
    }
    if (enc != NULL) {
      free(enc);
    }

    return true;
  }

  /*! \brief  Add drm information into sinf atom.
  
      Caller must take care that given sinf atom really coresponds to sample
      description atom which refers to encrypted samples.

      \param    originalFormat  input, 4CC code of the original data format.
      \param    sinf            input-output, sinf atom where to add drm information.
      \param    logger          input-output, message logger.

      \return Boolean indicating success or failure. In case of failure,
              logger's log will contain error message.
  */
  virtual bool Finish(UInt32T originalFormat, MP4Atom* sinf, DRMLogger& logger) {
	  MP4Atom* frma = sinf->FindChildAtom("frma");
    if (frma == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: no original format atom.");
      return false;
    }
	  MP4Property* data_format = 0;
	  frma->FindProperty("frma.data-format", &data_format, 0); 	
	  if (data_format == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: frma->SetDataFormat() error.");
      return false;
	  }
    ((MP4Integer32Property*)data_format)->SetValue(originalFormat);

    MP4Atom* imif = sinf->CreateAtom("imif");
    if (imif == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: could not create IPMP info atom.");
      return false;
    }
    sinf->AddChildAtom(imif);
    imif->Generate();

	  MP4Property* ipmp_desc = 0;
	  imif->FindProperty("imif.ipmp_desc", &ipmp_desc, 0); 	
	  if (ipmp_desc == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: missing IPMP descriptor property.");
      return false;
	  }
    MP4Descriptor* ipmpDesc = ((MP4DescriptorProperty*)ipmp_desc)->AddDescriptor(MP4IPMPDescrTag);
    if (ipmpDesc == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: could not create IPMP descriptor.");
      return false;
    }

	  MP4Property* IPMPDescriptorId = 0;
	  ipmpDesc->FindProperty("IPMPDescriptorId", &IPMPDescriptorId, 0); 	
	  if (IPMPDescriptorId == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: ipmpDesc->SetIPMPDescriptorIdentifier() error.");
      return false;
	  }
    ((MP4Integer8Property*)IPMPDescriptorId)->SetValue(255);

	  MP4Property* IPMPSType = 0;
	  ipmpDesc->FindProperty("IPMPSType", &IPMPSType, 0); 	
	  if (IPMPSType == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: ipmpDesc->SetIPMPDescriptorType() error.");
      return false;
	  }
    ((MP4Integer16Property*)IPMPSType)->SetValue(65535);

    std::string toolID = OPENIPMP_TOOLID;
    UInt32T ipmpDataSize = 1 + toolID.size() + 1 + contentID.size() + 1 + rightsHost.size() + 1;
    ByteT* ipmpData = (ByteT*)malloc(ipmpDataSize);
    if (ipmpData == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: memory allocation error.");
      return false;
    }
    //  Save the encryption method.
    *ipmpData = (ByteT)(AES128CTR);
    memcpy(ipmpData + 1, toolID.data(), toolID.size() + 1);
    memcpy(ipmpData + 1 + toolID.size() + 1, contentID.data(), contentID.size() + 1);
    memcpy(ipmpData + 1 + toolID.size() + 1 + contentID.size() + 1, rightsHost.data(),
      rightsHost.size() + 1);

	  MP4Property* IPMPData = 0;
	  ipmpDesc->FindProperty("IPMPData", &IPMPData, 0); 	
	  if (IPMPData == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: ipmpDesc->SetIPMPData() error.");
      return false;
	  }
    ((MP4BytesProperty*)IPMPData)->SetValue((u_int8_t*)ipmpData, ipmpDataSize);

    free(ipmpData);

    return true;
  }

private:
  AES128CTREncryptor* encryptor;
  std::string contentID;
  std::string rightsHost;
};

/*! \brief  Encrypts MP4 data using blowfish encryptor according to openIPMP
            specifications with Sinf signalling.

    Encrypts samples and adds information for decryption to MP4 stream.
*/
class BlowfishOpenIPMPMPEG4SinfDRMEncryptor: public IMPEG4SinfDRMEncryptor {
public:
  /*! \brief  Constructor.

      Mandatory tags in the XML file are:
       - ROOT.RightsHostInfo

      Here follows an example of a correctly formatted XML document.

      \verbatim

      <ROOT>
       <RightsHostInfo>localhost:8080</RightsHostInfo>
      </ROOT>

      \endverbatim

      \param      drm             input, DRM manager.
      \param      conID           input, content identifier.
      \param      xmlDoc          input, XML document.
      \param      logger          input-output, message logger.

      If constructor fails, it throws MPEG4DRMPluginException.
  */
  BlowfishOpenIPMPMPEG4SinfDRMEncryptor(IDRMEncManager* drm, std::string& conID,
      DRMPlugin::IXMLElement* xmlDoc, DRMLogger& logger): encryptor(), contentID(),
      rightsHost() {
    if (xmlDoc == 0) {
      logger.UpdateLog("BlowfishOpenIPMPMPEG4SinfDRMEncryptor::ctor: zero XML file.");
      throw MPEG4DRMPluginException();
    }
    encryptor = drm->CreateBlowfishEnc(conID, xmlDoc, logger);
    if (encryptor == 0) {
      logger.UpdateLog("BlowfishOpenIPMPMPEG4SinfDRMEncryptor::ctor: zero encryptor.");
      throw MPEG4DRMPluginException();
    }
    contentID = conID;
    try {
      rightsHost = xmlDoc->GetChildStrValue("RightsHostInfo");
    }
    catch (XMLException) {
      delete encryptor;
      logger.UpdateLog("BlowfishOpenIPMPMPEG4SinfDRMEncryptor::ctor: missing data in XML file.");
      throw MPEG4DRMPluginException();
    }
  }

  virtual ~BlowfishOpenIPMPMPEG4SinfDRMEncryptor() {
    if (encryptor != 0) delete encryptor;
  }

  /*! \brief  Encrypt sample sampleBuffer.
  
      Returns encrypted sample in encSampleData.

      \param    sampleBuffer      input, sample to be encrypted.
      \param    sampleSize        input, size of input sample.
      \param    encSampleData     output, encrypted sample.
      \param    encSampleLen      output, size of encrypted sample.
      \param    logger            input-output, message logger.

      \return Boolean indicating success or failure. In case of failure,
              logger's log will contain error message.
  */
  virtual bool EncryptSample(ByteT* sampleBuffer, UInt32T sampleSize,
      ByteT** encSampleData, UInt32T* encSampleLen, DRMLogger& logger) {
    return encryptor->Encrypt(sampleBuffer, sampleSize, encSampleData, encSampleLen,
      logger);
  }

  /*! \brief  Add drm information into sinf atom.
  
      Caller must take care that given sinf atom really coresponds to sample
      description atom which refers to encrypted samples.

      \param    originalFormat  input, 4CC code of the original data format.
      \param    sinf            input-output, sinf atom where to add drm information.
      \param    logger          input-output, message logger.

      \return Boolean indicating success or failure. In case of failure,
              logger's log will contain error message.
  */
  virtual bool Finish(UInt32T originalFormat, MP4Atom* sinf, DRMLogger& logger) {
	  MP4Atom* frma = sinf->FindChildAtom("frma");
    if (frma == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: no original format atom.");
      return false;
    }
	  MP4Property* data_format = 0;
	  frma->FindProperty("frma.data-format", &data_format, 0); 	
	  if (data_format == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: frma->SetDataFormat() error.");
      return false;
	  }
    ((MP4Integer32Property*)data_format)->SetValue(originalFormat);

    MP4Atom* imif = sinf->CreateAtom("imif");
    if (imif == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: could not create IPMP info atom.");
      return false;
    }
    sinf->AddChildAtom(imif);
    imif->Generate();

	  MP4Property* ipmp_desc = 0;
	  imif->FindProperty("imif.ipmp_desc", &ipmp_desc, 0); 	
	  if (ipmp_desc == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: missing IPMP descriptor property.");
      return false;
	  }
    MP4Descriptor* ipmpDesc = ((MP4DescriptorProperty*)ipmp_desc)->AddDescriptor(MP4IPMPDescrTag);
    if (ipmpDesc == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: could not create IPMP descriptor.");
      return false;
    }

	  MP4Property* IPMPDescriptorId = 0;
	  ipmpDesc->FindProperty("IPMPDescriptorId", &IPMPDescriptorId, 0); 	
	  if (IPMPDescriptorId == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: ipmpDesc->SetIPMPDescriptorIdentifier() error.");
      return false;
	  }
    ((MP4Integer8Property*)IPMPDescriptorId)->SetValue(255);

	  MP4Property* IPMPSType = 0;
	  ipmpDesc->FindProperty("IPMPSType", &IPMPSType, 0); 	
	  if (IPMPSType == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: ipmpDesc->SetIPMPDescriptorType() error.");
      return false;
	  }
    ((MP4Integer16Property*)IPMPSType)->SetValue(65535);

    std::string toolID = OPENIPMP_TOOLID;
    UInt32T ipmpDataSize = 1 + toolID.size() + 1 + contentID.size() + 1 + rightsHost.size() + 1;
    ByteT* ipmpData = (ByteT*)malloc(ipmpDataSize);
    if (ipmpData == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: memory allocation error.");
      return false;
    }
    //  Save the encryption method.
    *ipmpData = (ByteT)(BLOWFISH);
    memcpy(ipmpData + 1, toolID.data(), toolID.size() + 1);
    memcpy(ipmpData + 1 + toolID.size() + 1, contentID.data(), contentID.size() + 1);
    memcpy(ipmpData + 1 + toolID.size() + 1 + contentID.size() + 1, rightsHost.data(),
      rightsHost.size() + 1);

	  MP4Property* IPMPData = 0;
	  ipmpDesc->FindProperty("IPMPData", &IPMPData, 0); 	
	  if (IPMPData == 0) {
      logger.UpdateLog("AES128CTROpenIPMPMPEG4SinfDRMEncryptor::Finish: ipmpDesc->SetIPMPData() error.");
      return false;
	  }
    ((MP4BytesProperty*)IPMPData)->SetValue((u_int8_t*)ipmpData, ipmpDataSize);

    free(ipmpData);

    return true;
  }

private:
  BlowfishEncryptor* encryptor;
  std::string contentID;
  std::string rightsHost;
};

/*! \brief Protect sample using given encryptor.

    \param srcFile                input, source file handle.
    \param srcTrackId             input, source file track identifier.
    \param srcSampleId            input, source sample identifier.
    \param encryptor              input, DRM encryptor.
    \param dstFile                input, destination file handle.
    \param dstTrackId             input, destination file track identifier.
    \param dstSampleDuration      input, destination sample duration.
    \param logger                 input-output, message logger.

    \returns Boolean indicating success or failure.
*/
bool MP4EncryptSample(MP4FileHandle srcFile, MP4TrackId srcTrackId, 
	  MP4SampleId srcSampleId, IMPEG4SinfDRMEncryptor* encryptor, MP4FileHandle dstFile,
	  MP4TrackId dstTrackId, MP4Duration dstSampleDuration, DRMLogger& logger) {

	bool rc;
	u_int8_t* pBytes = NULL; 
	u_int32_t numBytes = 0;
	u_int8_t* encSampleData = NULL;
	u_int32_t encSampleLength = 0;
	MP4Duration sampleDuration;
	MP4Duration renderingOffset;
	bool isSyncSample;

	// Note: we leave it up to the caller to ensure that the
	// source and destination tracks are compatible.
	// i.e. copying audio samples into a video track 
	// is unlikely to do anything useful

	rc = MP4ReadSample(srcFile,	srcTrackId,	srcSampleId, &pBytes,	&numBytes, NULL,
		&sampleDuration, &renderingOffset, &isSyncSample);

	if (!rc) {
		return false;
	}

	if (dstFile == MP4_INVALID_FILE_HANDLE) {
		dstFile = srcFile;
	}
	if (dstTrackId == MP4_INVALID_TRACK_ID) {
		dstTrackId = srcTrackId;
	}
	if (dstSampleDuration != MP4_INVALID_DURATION) {
		sampleDuration = dstSampleDuration;
	}

  if (encryptor->EncryptSample((ByteT*)pBytes, numBytes, (ByteT**)&encSampleData,
      &encSampleLength, logger) == false) {
	  fprintf(stderr, "Can't encrypt the sample and add its header %u\n", srcSampleId);
  	free(pBytes);
    return false;
  }

	rc = MP4WriteSample(dstFile, dstTrackId, encSampleData, encSampleLength,
    sampleDuration, renderingOffset, isSyncSample);
		
	free(pBytes);

	if (encSampleData != NULL) {
    free(encSampleData);
  }
	
	return rc;
}

/*! \brief Protect track using given encryptor.

    Read all the samples from the track and protect them using given encryptor.
    When it's done, add DRM information to IMP4SinfAtom.

    \param srcFile                input, source file handle.
    \param srcTrackId             input, source file track identifier.
    \param encryptor              input, DRM encryptor.
    \param dstFile                input, destination file handle.
    \param dstTrackId             input, destination file track identifier.
    \param applyEdits             input, apply edits flag.
    \param logger                 input-output, message logger.

    \returns If failed MP4_INVALID_TRACK_ID, otherwise != MP4_INVALID_TRACK_ID.
*/
MP4TrackId MP4EncryptTrack(MP4FileHandle srcFile, MP4TrackId srcTrackId,
    IMPEG4SinfDRMEncryptor* encryptor, MP4FileHandle dstFile, MP4TrackId dstTrackId,
    bool applyEdits, DRMLogger& logger) {

  bool copySamples = true;	// LATER allow false => reference samples

  bool viaEdits = applyEdits && MP4GetTrackNumberOfEdits(srcFile, srcTrackId);

  MP4SampleId sampleId = 0;
  MP4SampleId numSamples = MP4GetTrackNumberOfSamples(srcFile, srcTrackId);

  MP4Timestamp when = 0;
  MP4Duration editsDuration = MP4GetTrackEditTotalDuration(srcFile, srcTrackId);

  while (true) {
    MP4Duration sampleDuration = MP4_INVALID_DURATION;

    if (viaEdits) {
      sampleId = MP4GetSampleIdFromEditTime(srcFile, srcTrackId, when, NULL,
					    &sampleDuration);

      // in theory, this shouldn't happen
      if (sampleId == MP4_INVALID_SAMPLE_ID) {
	      MP4DeleteTrack(dstFile, dstTrackId);
	      return MP4_INVALID_TRACK_ID;
      }

      when += sampleDuration;

      if (when >= editsDuration) {
      	break;
      }
    } else {
      sampleId++;
      if (sampleId > numSamples) {
      	break;
      }
    }

    bool rc = false;

    if (copySamples) {
      // encrypt and copy
      rc = MP4EncryptSample(srcFile, srcTrackId, sampleId, encryptor, dstFile,
        dstTrackId, sampleDuration, logger);
    } else {
      // not sure what these are - encrypt?
      rc = MP4ReferenceSample(srcFile, srcTrackId, sampleId, dstFile, dstTrackId,
			      sampleDuration);
    }

    if (!rc) {
      MP4DeleteTrack(dstFile, dstTrackId);
      return MP4_INVALID_TRACK_ID;
    }
  }

  MP4Track* track;
  MP4Atom* atom;
  MP4Atom* pStsdAtom;
  MP4Atom* sEntry;
  MP4Atom* sinf;

  track = ((MP4File*)srcFile)->GetTrack(srcTrackId);
  atom = track->GetTrakAtom();
	pStsdAtom = atom->FindAtom("trak.mdia.minf.stbl.stsd");
  if (pStsdAtom == 0) {
    MP4DeleteTrack(dstFile, dstTrackId);
    return MP4_INVALID_TRACK_ID;
  }
	sEntry = pStsdAtom->GetChildAtom(0);
  if (sEntry == 0) {
    MP4DeleteTrack(dstFile, dstTrackId);
    return MP4_INVALID_TRACK_ID;
  }
  std::string otypes = sEntry->GetType();
  if (otypes.size() != 4) {
    MP4DeleteTrack(dstFile, dstTrackId);
    return MP4_INVALID_TRACK_ID;
  }
  u_int32_t otype = ((otypes[0] << 24) | (otypes[1] << 16) | (otypes[2] << 8) | otypes[3]);

  track = ((MP4File*)dstFile)->GetTrack(dstTrackId);
  atom = track->GetTrakAtom();
	pStsdAtom = atom->FindAtom("trak.mdia.minf.stbl.stsd");
  if (pStsdAtom == 0) {
    MP4DeleteTrack(dstFile, dstTrackId);
    return MP4_INVALID_TRACK_ID;
  }
	sEntry = pStsdAtom->GetChildAtom(0);
  if (sEntry == 0) {
    MP4DeleteTrack(dstFile, dstTrackId);
    return MP4_INVALID_TRACK_ID;
  }
  sinf = sEntry->FindChildAtom("sinf");
  if (sinf == 0) {
    MP4DeleteTrack(dstFile, dstTrackId);
    return MP4_INVALID_TRACK_ID;
  }

  if (encryptor->Finish(otype, sinf, logger) == false) {
    MP4DeleteTrack(dstFile, dstTrackId);
    return MP4_INVALID_TRACK_ID;
  }

  return dstTrackId;
}

/*! \brief  Global handler for DRM protection.
*/
class DRMEncHandler {
public:
  DRMEncHandler(): doc(0), xmlDoc(0) {
    XMLFactory::Initialize();
    PublicCryptoFactory::Initialize();
    ThreadSyncFactory::Initialize();
  }

  ~DRMEncHandler() {
    if (doc != 0) {
      delete doc; doc = 0;
    }
    XMLFactory::Terminate();
    PublicCryptoFactory::Terminate();
    ThreadSyncFactory::Terminate();
  }

  IMPEG4SinfDRMEncryptor* GetEncryptor(const std::string& drmMethod,
      const std::string& encMethod, const std::string& xmlFileName,
      const std::vector<std::string>& sensitive,
      const std::vector<std::string>& node, const std::string& fName,
      MP4TrackId srcTrackId, DRMLogger& logger) {
    if (drmMethod == "") {
      fprintf(stderr, "GetEncryptor: Missing drm method!\n");
      return 0;
    }
    if (encMethod == "") {
      fprintf(stderr, "GetEncryptor: Missing encryption method!\n");
      return 0;
    }
    if (fName == "") {
      fprintf(stderr, "GetEncryptor: Missing input mp4 file!\n");
      return 0;
    }
    if (xmlDoc == 0) {
      xmlDoc = GetXML(xmlFileName, sensitive, node);
    }
  
    ContentInfoManager* infoManager = 0;
    IDRMEncManager* drmManager = 0;
    IMPEG4SinfDRMEncryptor* encryptor = 0;

    try {
      infoManager = ContentInfoManagerFactory::GetContentInfoManager(
        OPENIPMPDOIINFOMANAGER, xmlDoc, logger);
      if (infoManager == 0) {
        fprintf(stderr, "GetEncryptor: Error creating content info manager!\n");
        return 0;
      }
      char tID[11];
      itoa(srcTrackId, tID, 10);
      std::string title = fName + ".track" + std::string(tID);
      DRMPlugin::IXMLElement* elem = xmlDoc->GetChildElement("ContentTitle");
      if (elem == 0) {
        delete infoManager;
        fprintf(stderr, "GetEncryptor: Error in XML document!\n");
        return 0;
      }
      if (elem->SetStrValue(title) == false) {
        delete infoManager;
        fprintf(stderr, "GetEncryptor: Error in XML document!\n");
        return 0;
      }

      std::string info;
      if (infoManager->GetContentInfo(xmlDoc, info, logger) == false) {
        delete infoManager;
        fprintf(stderr, "GetEncryptor: Content info manager error!\n");
        return 0;
      }
      delete infoManager; infoManager = 0;

      if (drmMethod == "openipmp") {
        drmManager = OpenIPMPDRMEncManagerFactory::GetOpenIPMPDRMEncManager(xmlDoc,
          logger);
        if (drmManager == 0) {
          fprintf(stderr, "GetEncryptor: Error creating DRM manager!\n");
          return 0;
        }
        if (encMethod == "ctr") {
          encryptor = new AES128CTROpenIPMPMPEG4SinfDRMEncryptor(drmManager, info,
            xmlDoc, logger);
        } else if (encMethod == "bfs") {
          encryptor = new BlowfishOpenIPMPMPEG4SinfDRMEncryptor(drmManager, info,
            xmlDoc, logger);
        } else {
          delete drmManager;
          fprintf(stderr, "GetEncryptor: Wrong encryption method!\n");
          return 0;
        }
      } else if (drmMethod == "oma") {
        drmManager = OMADRMEncManagerFactory::GetOMADRMEncManager(xmlDoc, logger);
        if (drmManager == 0) {
          fprintf(stderr, "GetEncryptor: Error creating DRM manager!\n");
          return 0;
        }
        if (encMethod == "ctr") {
          encryptor = new AES128CTROMAMPEG4SinfDRMEncryptor(drmManager, info,
            xmlDoc, logger);
        } else if (encMethod == "cbc") {
          encryptor = new AES128CBCOMAMPEG4SinfDRMEncryptor(drmManager, info,
            xmlDoc, logger);
        } else {
          delete drmManager;
          fprintf(stderr, "GetEncryptor: Wrong encryption method!\n");
          return 0;
        }
      } else if (drmMethod == "isma") {
        drmManager = ISMADRMEncManagerFactory::GetISMADRMEncManager(xmlDoc, logger);
        if (drmManager == 0) {
          fprintf(stderr, "GetEncryptor: Error creating DRM manager!\n");
          return 0;
        }
        if (encMethod == "icm") {
          encryptor = new AES128ICMISMAMPEG4SinfDRMEncryptor(drmManager, info,
            xmlDoc, logger);
        } else {
          delete drmManager;
          fprintf(stderr, "GetEncryptor: Wrong encryption method!\n");
          return 0;
        }
      } else {
        fprintf(stderr, "GetEncryptor: Wrong DRM method!\n");
        return 0;
      }
      //  When and where to add license?
      drmManager->AddLicense(info, xmlDoc, logger);
      delete drmManager; drmManager = 0;
    }
    catch (...) {
      if (infoManager != 0) delete infoManager;
      if (drmManager != 0) delete drmManager;
      if (encryptor != 0) delete encryptor;
      fprintf(stderr, "GetEncryptor: Unknown error!\n");
      return 0;
    }

    return encryptor;
  }

  DRMPlugin::IXMLElement* GetXML(const std::string& xmlFileName,
      const std::vector<std::string>& sensitive,
      const std::vector<std::string>& node) {
    if (xmlFileName != "") {
      if ((doc = XMLFactory::DecodeDocumentFromFile(xmlFileName)) == 0) {
        fprintf(stderr, "%s: Invalid XML configuration file!\n", ProgName);
        return 0;
      }

      if ((xmlDoc = doc->GetRootElement()) == 0) {
        delete doc; doc = 0; xmlDoc = 0;
        return 0;
      }

      for (unsigned int count = 0; count < sensitive.size(); count++) {
        char* slash = strchr(sensitive[count].begin(), '/');
        if (slash == NULL) {
          delete doc; doc = 0; xmlDoc = 0;
          return 0;
        }
        std::string path = std::string(sensitive[count].begin(), slash);
        std::string value = std::string(++slash, sensitive[count].end());
        DRMPlugin::IXMLElement* elem = xmlDoc->GetChildElement(path);
        if (elem == 0) {
          delete doc; doc = 0; xmlDoc = 0;
          return 0;
        }
        if (elem->SetStrValue(value) == false) {
          delete doc; doc = 0; xmlDoc = 0;
          return 0;
        }
      }

      for (count = 0; count < node.size(); count++) {
        char* slash = strchr(node[count].begin(), '/');
        if (slash == NULL) {
          delete doc; doc = 0; xmlDoc = 0;
          return 0;
        }
        std::string name = std::string(node[count].begin(), slash);
        std::string value = std::string(++slash, node[count].end());
        if (xmlDoc->AddChildElement(name, value) == 0) {
          delete doc; doc = 0; xmlDoc = 0;
          return 0;
        }
      }
    }

    return xmlDoc;
  }

private:
  DRMPlugin::IXMLDocument* doc;
  DRMPlugin::IXMLElement* xmlDoc;
  DRMLogger logger;
};

/*!  \brief Protect track.

    Given a source track in a source file, make an encrypted copy of the track
    in the destination file, including sample encryption.

    Mandatory tags in the XML document are dependent on the DRM and content info
    managers used.

    If drmMethod is "openipmp", then encMethod must be "bfs" or "ctr".
    If drmMethod is "oma", then encMethod must be "cbc" or "ctr".
    If drmMethod is "isma", then encMethod must be "icm".
    Other values for drmMethod are not supported.

    \param    drmMethod                     input, DRM method.
    \param    encMethod                     input, encryption method.
    \param    xmlFileName                   input, XML configuration file name.
    \param    sensitive                     input, sensitive data.
    \param    node                          input, sensitive data.
    \param    srcFile                       input, source file handle.
    \param    srcTrackId                    input, source file track identifier.
    \param    fName                         input, name of the source mp4 file to be protected.
    \param    dstFile                       input, destination file handle.
    \param    applyEdits                    input, apply edits flag.
    \param    dstHintTrackReferenceTrack    input, destination hint reference track
                                            identifier.

    \returns If failed MP4_INVALID_TRACK_ID, otherwise != MP4_INVALID_TRACK_ID.
*/
MP4TrackId EncryptTrack(const std::string& drmMethod, const std::string& encMethod,
    const std::string& xmlFileName, const std::vector<std::string>& sensitive,
    const std::vector<std::string>& node, MP4FileHandle srcFile, MP4TrackId srcTrackId,
    const std::string& fName, MP4FileHandle dstFile, bool applyEdits,
    MP4TrackId dstHintTrackReferenceTrack) {

  static DRMLogger logger;
  static DRMEncHandler handler;

  IMPEG4SinfDRMEncryptor* encryptor = handler.GetEncryptor(drmMethod, encMethod,
    xmlFileName, sensitive, node, fName, srcTrackId, logger);

  if (encryptor == 0) {
    return MP4_INVALID_TRACK_ID;
  }

  // Patch with dummy isma parameters.
  mp4v2_ismacrypParams icPp;

  MP4TrackId dstTrackId = MP4EncAndCloneTrack(srcFile, srcTrackId, &icPp,
    dstFile, dstHintTrackReferenceTrack);

  if (dstTrackId == MP4_INVALID_TRACK_ID) {
    delete encryptor;
    return MP4_INVALID_TRACK_ID;
  }

  if (MP4EncryptTrack(srcFile, srcTrackId, encryptor, dstFile, dstTrackId,
      applyEdits, logger) == MP4_INVALID_TRACK_ID) {
    delete encryptor;
    MP4DeleteTrack(dstFile, dstTrackId);
    return MP4_INVALID_TRACK_ID;
  }
  delete encryptor;

  return dstTrackId;
}

/*****************************************************************************/
