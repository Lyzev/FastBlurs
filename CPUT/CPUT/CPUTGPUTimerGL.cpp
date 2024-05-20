


#include "CPUTGPUTimerGL.h"
#include "CPUT_OGL.h"

#include <algorithm>

std::vector<CPUTGPUTimerGL*>     CPUTGPUTimerGL::s_instances;
unsigned int                     CPUTGPUTimerGL::s_lastFrameID                = 0;
bool                             CPUTGPUTimerGL::s_frameActive                = false;

std::vector<GLuint>				CPUTGPUTimerGL::s_timerQueries;


std::vector<CPUTGPUQueryGL*>     CPUTGPUQueryGL::s_instances;
unsigned int                                CPUTGPUQueryGL::s_lastFrameID                = 0;
bool                                   CPUTGPUQueryGL::s_frameActive                = false;
std::vector<CPUTGPUQueryGL::SPerfQueryType> CPUTGPUQueryGL::m_QueryTypes;




#ifdef CPUT_OS_WINDOWS
#define OSSleep Sleep
#else
#include <unistd.h>
#define OSSleep sleep
#endif

CPUTGPUTimerGL::CPUTGPUTimerGL()
{
   s_instances.push_back( this );

   memset( m_history, 0, sizeof(m_history) );
   for( int i = 0; i < (int)c_historyLength; i++ )
   {
      m_history[i].TimingResult   = -1.0;
	  m_history[i].StartData = 0;
	  m_history[i].StopData = 0;
   }

   m_historyLastIndex         = 0;
   m_active                   = false;
   m_lastTime                 = 0.0f;
   m_avgTime                  = 0.0f;
}

CPUTGPUTimerGL::~CPUTGPUTimerGL()
{
   std::vector<CPUTGPUTimerGL*>::iterator it = std::find( s_instances.begin(), s_instances.end(), this );
   if( it != s_instances.end() )
   {
      s_instances.erase( it );
   }
   else
   {
      // this instance not found in the global list?
      assert( false );
   }
}






void CPUTGPUTimerGL::OnDeviceAndContextCreated( )

{
   s_lastFrameID                 = 0;
   s_frameActive                 = false;
}

void CPUTGPUTimerGL::OnDeviceAboutToBeDestroyed( )
{
   FrameFinishQueries( true );

   s_timerQueries.clear();
}

void CPUTGPUTimerGL::FrameFinishQueries( bool forceAll )
{
   assert( !s_frameActive );

   for( int i = 0; i < (int)s_instances.size(); i++ )
      s_instances[i]->FinishQueries( forceAll );
}


GLuint  CPUTGPUTimerGL::GetTimerQuery2()
{
	GLuint queryID;
	if( s_timerQueries.size() == 0 )
	{
	  GL_CHECK(glGenQueries(1,&queryID));
   }
   else
   {
      queryID = s_timerQueries.back();
      s_timerQueries.pop_back();
   }
   return queryID;
}


void CPUTGPUTimerGL::OnFrameStart()
{
   FrameFinishQueries( false );

   assert( !s_frameActive );

   s_frameActive = true;
   s_lastFrameID++;
}

void CPUTGPUTimerGL::OnFrameEnd()
{
   assert( s_frameActive );
   s_frameActive = false;
}


static bool GetQueryDataHelper2( bool loopUntilDone, GLuint query, GLuint64* startTime )
{
   if( query == NULL )
      return false;

   int attempts = 0;
   do
   {
		// wait until the results are available
		GLint stopTimerAvailable = 0;

		glGetQueryObjectiv(query,GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);

		if (stopTimerAvailable == 0)
		{
			// results not available yet - should try again later
		}
		else
		{
			glGetQueryObjectui64v(query, GL_QUERY_RESULT, startTime);
			return true;
		}
		attempts++; 
		if( attempts > 1000 ) OSSleep(0);
		if( attempts > 100000 ) { ASSERT( false, L"CPUTGPUTimerDX11.cpp - Infinite loop while doing s_immediateContext->GetData() - this shouldn't happen. " _CRT_WIDE(__FILE__) L", line: " _CRT_WIDE(_CRT_STRINGIZE(__LINE__)) ); return false; }
   } while ( loopUntilDone);
   return false;
}

void CPUTGPUTimerGL::FinishQueries( bool forceAll )
{
	
   assert( !m_active );

   int dataGathered = 0;
   m_avgTime = 0.0;
   m_NewRetires = 0;

   // get data from previous frames queries if available
   for( int i = 0; i < (int)c_historyLength; i++ )
   {
      int safeIndex = (i % c_historyLength);

      GPUTimerInfo & item = m_history[safeIndex];

      bool tryGather = true;

      if( item.QueryActive )
      {
         bool loopUntilDone = ((s_lastFrameID - item.FrameID) >= c_dataQueryMaxLag) || forceAll;

		GLuint64 startTime = 0, stopTime = 0;
		 

		 GetQueryDataHelper2( loopUntilDone, item.StartQuery2, &startTime );
		 GetQueryDataHelper2( loopUntilDone, item.StopQuery2, &stopTime );

		 if(startTime && stopTime )
		 {
			 item.TimingResult = (double)(stopTime-startTime)/1000000000.0f;
			item.QueryActive = false;
			m_NewRetires++;
			if(item.FrameID>m_lastFrameID)
			{
				m_lastFrameID = item.FrameID;
				m_lastTime = item.TimingResult;
			}
		 }
      }

	if( (!item.QueryActive) && (item.TimingResult != -1.0) )
	{
		dataGathered++;
		m_avgTime += item.TimingResult;
		
	}
   }



   if( dataGathered == 0 )
      m_avgTime = 0.0f;
   else
      m_avgTime /= (double)dataGathered;
}



void CPUTGPUTimerGL::Start()
{
	assert( s_frameActive );
	assert( !m_active );

	m_historyLastIndex = (m_historyLastIndex+1)%c_historyLength;
   
	assert( !m_history[m_historyLastIndex].QueryActive );

   m_history[m_historyLastIndex].StartQuery2     = GetTimerQuery2();
   glQueryCounter(m_history[m_historyLastIndex].StartQuery2, GL_TIMESTAMP);

	m_history[m_historyLastIndex].QueryActive    = true;
	m_history[m_historyLastIndex].FrameID        = s_lastFrameID;
	m_active = true;
    m_history[m_historyLastIndex].TimingResult   = -1.0;
}

void CPUTGPUTimerGL::Stop()
{
   assert( s_frameActive );
   assert( m_active );
   assert( m_history[m_historyLastIndex].QueryActive );

   m_history[m_historyLastIndex].StopQuery2      = GetTimerQuery2();
   glQueryCounter(m_history[m_historyLastIndex].StopQuery2, GL_TIMESTAMP);

   m_active = false;
}


//////////////////////////////////////////////////

CPUTGPUQueryGL::CPUTGPUQueryGL()
{
   s_instances.push_back( this );

   memset( m_history, 0, sizeof(m_history) );
   for( int i = 0; i < (int)c_historyLength; i++ )
   {
      m_history[i].TimingResult   = -1.0;
	  m_history[i].StartData = 0;
	  m_history[i].StopData = 0;
   }

   m_historyLastIndex         = 0;
   m_active                   = false;
   m_lastTime                 = 0.0f;
   m_avgTime                  = 0.0f;
}

CPUTGPUQueryGL::~CPUTGPUQueryGL()
{
   std::vector<CPUTGPUQueryGL*>::iterator it = std::find( s_instances.begin(), s_instances.end(), this );
   if( it != s_instances.end() )
   {
      s_instances.erase( it );
   }
   else
   {
      // this instance not found in the global list?
      assert( false );
   }
}


void CPUTGPUQueryGL::ProcessCounterType(SPerfCounter& counter, char* buf,void* value, int valueSize)
{
	assert(counter.Size == valueSize);
	memcpy(value, buf + counter.Offset, counter.Size);
}




void CPUTGPUQueryGL::ProcessQueryType(GLuint queryTypeId)
{
	char queryName[256]; // don't know what the upper limit should be
	GLuint bufferSize;
	GLuint numCounters;
	GLuint maxQueries;
	GLuint unknown; // don't know what this is for; always seems to be set to 1

	GL_CHECK(glGetPerfQueryInfoINTEL(queryTypeId, sizeof(queryName), queryName, &bufferSize, &numCounters, &maxQueries, &unknown));

    SPerfQueryType query;  
    query.queryTypeId = queryTypeId;  
    query.name = queryName;               
	query.counterBufferSize = bufferSize;

	DEBUG_PRINT( _L("%s  : %d\n"), queryName, numCounters );

	for (GLuint counterId = 1; counterId <= numCounters; ++counterId)
	{
		char counterName[256];  // don't know what the upper limit should be
		char counterDesc[1024]; // don't know what the upper limit should be
		GLuint counterOffset;
		GLuint counterSize;
		GLuint counterCategory;   // one of INTEL_PERFQUERIES_CATEGORY_*
		GLuint counterType;       // one of INTEL_PERFQUERIES_TYPE_*
		GLuint64 unknown2; // don't know what this is for; seems to be set to 0 or 1

		GL_CHECK(glGetPerfCounterInfoINTEL(queryTypeId, counterId,
			sizeof(counterName), counterName,
			sizeof(counterDesc), counterDesc,
			&counterOffset, &counterSize, &counterCategory, &counterType, &unknown2));

		DEBUG_PRINT(  _L("%s %d : %d\n"), counterName, counterSize, counterOffset );

        SPerfCounter counter;  
		counter.Name = counterName;  
		counter.Desc = counterDesc;  
		counter.Offset = counterOffset;  
		counter.Size = counterSize;  
		counter.Type = counterType;  
		query.counters[counterName]= counter;
	}
	m_QueryTypes.push_back(query);
}

void CPUTGPUQueryGL::OnDeviceAndContextCreated( )

{
   s_lastFrameID                 = 0;
   s_frameActive                 = false;

   if( glGetFirstPerfQueryIdINTEL != NULL )
   {
    GLuint queryId;
	GL_CHECK(glGetFirstPerfQueryIdINTEL(&queryId));
	while (queryId != 0)
	{
		ProcessQueryType(queryId);

		GL_CHECK(glGetNextPerfQueryIdINTEL(queryId, &queryId));
	}
  }
}

void CPUTGPUQueryGL::OnDeviceAboutToBeDestroyed( )
{
    if( glGetFirstPerfQueryIdINTEL != NULL )
    {
        FrameFinishQueries( true );

        for (size_t i = 0; i < m_QueryTypes.size(); ++i)  
		    for (size_t j = 0; j < m_QueryTypes[i].freeQueries.size(); ++j)  
			    GL_CHECK(glDeletePerfQueryINTEL(m_QueryTypes[i].freeQueries[j]));

    }
}

void CPUTGPUQueryGL::FrameFinishQueries( bool forceAll )
{
   assert( !s_frameActive );

   for( int i = 0; i < (int)s_instances.size(); i++ )
      s_instances[i]->FinishQueries( forceAll );
}

GLuint  CPUTGPUQueryGL::GetTimerQuery(size_t queryIdx)
{
 //   ENSURE(queryIdx < m_QueryTypes.size());  
		 
	if (m_QueryTypes[queryIdx].freeQueries.empty())  
	{  
		GLuint id;  
		GL_CHECK(glCreatePerfQueryINTEL(m_QueryTypes[queryIdx].queryTypeId, &id));  
		return id;  
	}  
		 
	GLuint id = m_QueryTypes[queryIdx].freeQueries.back();  
	m_QueryTypes[queryIdx].freeQueries.pop_back();  
		 return id;
}




void CPUTGPUQueryGL::OnFrameStart()
{
    if( glGetFirstPerfQueryIdINTEL != NULL )
    {
   FrameFinishQueries( false );

   assert( !s_frameActive );

   s_frameActive = true;
   s_lastFrameID++;
    }
}

void CPUTGPUQueryGL::OnFrameEnd()
{
    if( glGetFirstPerfQueryIdINTEL != NULL )
    {
   assert( s_frameActive );
   s_frameActive = false;
    }
}



static bool GetQueryDataHelper( bool loopUntilDone, GLuint query, void * data, int dataSize )
{

   if( query == NULL )
      return false;

   int attempts = 0;
   do
   {
		GLuint length;
		GL_CHECK(glGetPerfQueryDataINTEL(query, INTEL_PERFQUERIES_NONBLOCK, dataSize, data, &length));
		if (length == 0)
		{
			// results not available yet - should try again later
		}
		else
		{
			if(length != dataSize) // always seems to be true
			{
				DEBUG_PRINT( _L("Woops %d : %d"), length, dataSize );
				assert(0);
			}
	//		ParseCounterValues(buffer);
			return true;
		}
		attempts++; 
		if( attempts > 1000 ) OSSleep(0);
		if( attempts > 100000 ) { ASSERT( false, L"CPUTGPUTimerDX11.cpp - Infinite loop while doing s_immediateContext->GetData() - this shouldn't happen. " _CRT_WIDE(__FILE__) L", line: " _CRT_WIDE(_CRT_STRINGIZE(__LINE__)) ); return false; }
   } while ( loopUntilDone);
   return false;
}


void CPUTGPUQueryGL::FinishQueries( bool forceAll )
{
	
   assert( !m_active );

   int dataGathered = 0;
   m_avgTime = 0.0;

   // get data from previous frames queries if available
   for( int i = 0; i < (int)c_historyLength; i++ )
   {
      int safeIndex = (i % c_historyLength);

      GPUTimerInfo & item = m_history[safeIndex];

      bool tryGather = true;

      if( item.QueryActive )
      {
         bool loopUntilDone = ((s_lastFrameID - item.FrameID) >= c_dataQueryMaxLag) || forceAll;

		 uint64 TotalTime = 0;

		 if( GetQueryDataHelper( loopUntilDone, item.StartQuery,item.StartData, item.StartDataSize  ) )
         {
			m_QueryTypes[0].freeQueries.push_back(item.StartQuery);
 			ProcessCounterType(CPUTGPUQueryGL::m_QueryTypes[0].counters["TotalTime"],(char*)item.StartData,&TotalTime,sizeof(TotalTime));
         }
		 
		 if(TotalTime )
		 {
			item.TimingResult = TotalTime /1000000000.0f;
			item.QueryActive = false;
			if(item.FrameID>m_lastFrameID)
			{
				m_lastFrameID = item.FrameID;
				m_lastTime = item.TimingResult;
			}
		 }

      }
	  
		if( (!item.QueryActive) && (item.TimingResult != -1.0) )
		{
			dataGathered++;
			m_avgTime += item.TimingResult;

			free(item.StartData);
			item.StartData = NULL;
		}
	  
	  
   }

   if( dataGathered == 0 )
      m_avgTime = 0.0f;
   else
      m_avgTime /= (double)dataGathered;
}



void CPUTGPUQueryGL::Start()
{
	assert( s_frameActive );
	assert( !m_active );

	m_historyLastIndex = (m_historyLastIndex+1)%c_historyLength;
   
	assert( !m_history[m_historyLastIndex].QueryActive );

	
	m_history[m_historyLastIndex].StartQuery     = GetTimerQuery(0);
	m_history[m_historyLastIndex].StartDataSize = CPUTGPUQueryGL::m_QueryTypes[0].counterBufferSize;
    m_history[m_historyLastIndex].StartData =  malloc(m_history[m_historyLastIndex].StartDataSize);
	
   
	m_history[m_historyLastIndex].StopQuery		= NULL;
	GL_CHECK(glBeginPerfQueryINTEL(m_history[m_historyLastIndex].StartQuery));
	

	m_history[m_historyLastIndex].QueryActive    = true;
	m_history[m_historyLastIndex].FrameID        = s_lastFrameID;
	m_active = true;
    m_history[m_historyLastIndex].TimingResult   = -1.0;
}

void CPUTGPUQueryGL::Stop()
{
   assert( s_frameActive );
   assert( m_active );
   assert( m_history[m_historyLastIndex].QueryActive );

   
    m_history[m_historyLastIndex].StopQuery      = GetTimerQuery(0);
	GL_CHECK(glEndPerfQueryINTEL(m_history[m_historyLastIndex].StartQuery));
	

  m_active = false;
}