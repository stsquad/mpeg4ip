#ifndef __SimplePlayListElement__
#define __SimplePlayListElement__

#include <string.h>
#include "MyAssert.h"

class SimplePlayListElement {

	public:
		SimplePlayListElement( const char * name )
		{
			mElementName = new char[ strlen(name) + 1 ];
			
			Assert( mElementName );
			if( mElementName )
				strcpy( mElementName, name );
			
			mElementWeight = -1;
			
			mWasPlayed = false;
			
		}		
		
		virtual ~SimplePlayListElement() 
		{ 
			if ( mElementName )  
				delete [] mElementName;
		}
		
		int	 	mElementWeight;
		bool	mWasPlayed;
		char 	*mElementName;

};

#endif
