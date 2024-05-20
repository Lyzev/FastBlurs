
#ifndef __CPUTGPUTimer_h__
#define __CPUTGPUTimer_h__

#include "CPUT.h"
#include "CPUTGPUTimer.h"

#include <vector>
#include <map>


class CPUTGPUTimerGL : public CPUTGPUTimer
{
public:
	
private:	

    static const int                        c_dataQueryMaxLag       = 2;                       // in frames
    static const int                        c_historyLength         = 1+c_dataQueryMaxLag;   
    static unsigned int                     s_lastFrameID;
    static bool                             s_frameActive;
	static std::vector<CPUTGPUTimerGL*>		s_instances;

    static std::vector<GLuint>				s_timerQueries;

    public:

    static void                               OnFrameStart();
    static void                               OnFrameEnd();


	static void                               OnDeviceAndContextCreated( );
    static void                               OnDeviceAboutToBeDestroyed( );

private:
    static void                               FrameFinishQueries( bool forceAll );
	GLuint									  GetTimerQuery2();
public:
   
    // NON-STATIC bit
    struct GPUTimerInfo
    {
        unsigned int							FrameID;
       GLuint									StartQuery;
       GLuint									StartQuery2;
       GLuint									StopQuery2;
	   GLuint									StartDataSize;
	   void*		StartData;
	   void*		StopData;
       GLuint									StopQuery;
       bool										QueryActive;
       double									TimingResult;
    };
   
    int                                       m_historyLastIndex;
    GPUTimerInfo                              m_history[c_historyLength];
    bool                                      m_active;
   
    double                                    m_lastTime;
    double                                    m_avgTime;
	unsigned int							  m_lastFrameID;
	unsigned int							  m_NewRetires;
   
   
                                              CPUTGPUTimerGL();
                                              ~CPUTGPUTimerGL();

    double                                    GetAvgTime( ) const     { return m_avgTime; }
   
    void                                      Start();
    void                                      Stop();
   
private:
    void                                      FinishQueries( bool forceAll );
};


class CPUTGPUQueryGL : public CPUTGPUTimer
{
public:

	struct SPerfCounter
	{
		std::string Name; 
		std::string Desc;
		GLuint Offset;
		GLuint Size;
		GLuint Category;   // one of INTEL_PERFQUERIES_CATEGORY_*
		GLuint Type;       // one of INTEL_PERFQUERIES_TYPE_*
	};
	struct  SPerfQueryType
	{
		GLuint  queryTypeId; 
		std::string  name; 
		GLuint  counterBufferSize; 
//		std::vector<  SPerfCounter >  counters; 
		std::map< std::string, SPerfCounter >  counters; 
		std::vector< GLuint >  freeQueries;
	};
private:	
	static std::vector<SPerfQueryType> m_QueryTypes;

    static const int                        c_dataQueryMaxLag       = 4;                       // in frames
    static const int                        c_historyLength         = 1+c_dataQueryMaxLag;   
    static unsigned int                     s_lastFrameID;
    static bool                             s_frameActive;
	static std::vector<CPUTGPUQueryGL*>		s_instances;

    public:

    static void                               OnFrameStart();
    static void                               OnFrameEnd();


	static void                               OnDeviceAndContextCreated( );
    static void                               OnDeviceAboutToBeDestroyed( );

	static void ProcessQueryType(GLuint queryId);
	static void ProcessCounterType(SPerfCounter& counter, char* buf,void* value, int valueSize);

private:
    static void                               FrameFinishQueries( bool forceAll );
	GLuint									  GetTimerQuery(size_t queryIdx);

public:
   
    // NON-STATIC bit
    struct GPUTimerInfo
    {
        unsigned int							FrameID;
       GLuint									StartQuery;
       GLuint									StartQuery2;
       GLuint									StopQuery2;
	   GLuint									StartDataSize;
	   void*		StartData;
	   void*		StopData;
       GLuint									StopQuery;
       bool										QueryActive;
       double									TimingResult;
    };
   
    int                                       m_historyLastIndex;
    GPUTimerInfo                              m_history[c_historyLength];
    bool                                      m_active;
   
    double                                    m_lastTime;
    double                                    m_avgTime;
	unsigned int							m_lastFrameID;
   
   
                                              CPUTGPUQueryGL();
                                              ~CPUTGPUQueryGL();

    double                                    GetAvgTime( ) const     { return m_avgTime; }
   
    void                                      Start();
    void                                      Stop();
   
private:
    void                                      FinishQueries( bool forceAll );
};

class CPUTGPUProfilerGL_AutoScopeProfile
{
    CPUTGPUTimerGL &                    m_profiler;
	CPUTGPUQueryGL &                    m_Query;
    const bool                          m_doProfile;

public:
    CPUTGPUProfilerGL_AutoScopeProfile( CPUTGPUTimerGL & profiler, CPUTGPUQueryGL &query, bool doProfile = true ) : m_profiler(profiler), m_doProfile(doProfile) , m_Query(query)       
    { 
        if( m_doProfile ) m_profiler.Start(); 
        if( m_doProfile ) m_Query.Start(); 
    }
    ~CPUTGPUProfilerGL_AutoScopeProfile( )                                             
    { 
        if( m_doProfile ) m_profiler.Stop(); 
        if( m_doProfile ) m_Query.Stop(); 
    }
};

#endif // __CPUTGPUTimer_h__
