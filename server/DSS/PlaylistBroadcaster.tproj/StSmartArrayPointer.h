#ifndef __StSmartArrayPointer__
#define __StSmartArrayPointer__

template <class T>
class StSmartArrayPointer
{
	public:
		StSmartArrayPointer(T* victim) : fT(victim)  {}
		~StSmartArrayPointer() { delete [] fT; }
		
		void SetObject(T* victim) 
		{ 
			//can't use a normal assert here because "Assert.h" eventually includes this file....
			#ifdef ASSERT
				//char s[65]; 
				if (fT != NULL) printf ("_Assert: StSmartArrayPointer::SetObject() %s, %d\n", __FILE__, __LINE__); 
			#endif 

			fT = victim; 
		}
		
		T* GetObject() { return fT; }
		
		operator T*() { return fT; }
	
	private:
	
		T* fT;
};

typedef StSmartArrayPointer<char> OSCharArrayDeleter;
#endif
