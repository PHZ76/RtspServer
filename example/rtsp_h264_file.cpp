// RTSP-Server

#include "xop/RtspServer.h"
#include "net/NetInterface.h"
#include "net/Timer.h"
#include <thread>
#include <memory>
#include <iostream>
#include <string>

class H264File
{
public:
    H264File(int bufSize=500000);
    ~H264File();

    bool open(const char *path);
    void close();

    bool isOpened() const
    {
        return (m_file != NULL);
    }

    int readFrame(char *inBuf, int inBufSize, bool *bEndOfFrame);
    
private:
    FILE *m_file = NULL;
    char *m_buf = NULL;
    int m_bufSize = 0;
    int m_bytesUsed = 0;
    int m_count = 0;
};

void snedFrameThread(xop::RtspServer* rtspServer, xop::MediaSessionId sessionId, H264File* h264File);

int main(int argc, char **argv)
{	
    if(argc != 2)
    {
        printf("Usage: %s test.h264\n", argv[0]);
        return 0;
    }

    H264File h264File;
    if(!h264File.open(argv[1]))
    {
        printf("Open %s failed.\n", argv[1]);
        return 0;
    }

    std::string ip = "127.0.0.1";
    std::string rtspUrl;

    std::shared_ptr<xop::EventLoop> eventLoop(new xop::EventLoop());  
    xop::RtspServer server(eventLoop.get(), "0.0.0.0", 554);  

#ifdef AUTH_CONFIG
	server.setAuthConfig("-_-", "admin", "12345");
#endif

    xop::MediaSession *session = xop::MediaSession::createNew("live"); 
    rtspUrl = "rtsp://" + ip + ":554/" + session->getRtspUrlSuffix();

    session->addMediaSource(xop::channel_0, xop::H264Source::createNew()); 
	//session->startMulticast();  /* enable multicast */
	session->setNotifyCallback([] (xop::MediaSessionId sessionId, uint32_t clients){
		std::cout << "Number of rtsp client : " << clients << std::endl;
	});
   
    xop::MediaSessionId sessionId = server.addMeidaSession(session); 
         
    std::thread t1(snedFrameThread, &server, sessionId, &h264File); 
    t1.detach(); 

    std::cout << "Play URL: " <<rtspUrl << std::endl;

	while (1)
	{
		xop::Timer::sleep(100);
	}

    getchar();
    return 0;
}

void snedFrameThread(xop::RtspServer* rtspServer, xop::MediaSessionId sessionId, H264File* h264File)
{       
    int bufSize = 500000;
    uint8_t *frameBuf = new uint8_t[bufSize];
        
    while(1)
    {
        bool bEndOfFrame;
        int frameSize = h264File->readFrame((char*)frameBuf, bufSize, &bEndOfFrame);
        if(frameSize > 0)
        {
            xop::AVFrame videoFrame = {0};
            videoFrame.type = 0; 
            videoFrame.size = frameSize; 
            videoFrame.timestamp = xop::H264Source::getTimeStamp(); 
            videoFrame.buffer.reset(new uint8_t[videoFrame.size]);    
            memcpy(videoFrame.buffer.get(), frameBuf, videoFrame.size);
            rtspServer->pushFrame(sessionId, xop::channel_0, videoFrame);
        }
        else
        {
            break;
        }
      
        xop::Timer::sleep(40); 
    }

    delete frameBuf;
}

H264File::H264File(int bufSize)
    : m_bufSize(bufSize)
{
    m_buf = new char[m_bufSize];
}

H264File::~H264File()
{
    delete m_buf;
}

bool H264File::open(const char *path)
{
    m_file = fopen(path, "rb");
    if(m_file == NULL)
    {      
       return false;
    }

    return true;
}

void H264File::close()
{
    if(m_file)
    {
        fclose(m_file);
        m_file = NULL;
        m_count = 0;
        m_bytesUsed = 0;
    }
}

int H264File::readFrame(char *inBuf, int inBufSize, bool *bEndOfFrame)
{
    if(m_file == NULL)
    {
        return -1;
    }

    int bytesRead = (int)fread(m_buf, 1, m_bufSize, m_file);
    if(bytesRead == 0)
    {
        fseek(m_file, 0, SEEK_SET); 
        m_count = 0;
        m_bytesUsed = 0;
        bytesRead = (int)fread(m_buf, 1, m_bufSize, m_file);
        if(bytesRead == 0)        
        {            
            this->close();
            return -1;
        }
    }

    bool bFindStart = false, bFindEnd = false;

    int i = 0, startCode = 3;
    *bEndOfFrame = false;
    for (i=0; i<bytesRead-5; i++)
    {
        if(m_buf[i] == 0 && m_buf[i+1] == 0 && m_buf[i+2] == 1)
        {
            startCode = 3;
        }
        else if(m_buf[i] == 0 && m_buf[i+1] == 0 && m_buf[i+2] == 0 && m_buf[i+3] == 1)
        {
            startCode = 4;
        }
        else 
        {
            continue;
        }
        
        if (((m_buf[i+startCode]&0x1F) == 0x5 || (m_buf[i+startCode]&0x1F) == 0x1) &&
             ((m_buf[i+startCode+1]&0x80) == 0x80))				 
        {
            bFindStart = true;
            i += 4;
            break;
        }
    }

    for (; i<bytesRead-5; i++)
    {
        if(m_buf[i] == 0 && m_buf[i+1] == 0 && m_buf[i+2] == 1)
        {
            startCode = 3;
        }
        else if(m_buf[i] == 0 && m_buf[i+1] == 0 && m_buf[i+2] == 0 && m_buf[i+3] == 1)
        {
            startCode = 4;
        }
        else 
        {
            continue;
        }
        
        if (((m_buf[i+startCode]&0x1F) == 0x7) || ((m_buf[i+startCode]&0x1F) == 0x8) 
            || ((m_buf[i+startCode]&0x1F) == 0x6)|| (((m_buf[i+startCode]&0x1F) == 0x5 
            || (m_buf[i+startCode]&0x1F) == 0x1) &&((m_buf[i+startCode+1]&0x80) == 0x80)))
        {
            bFindEnd = true;
            break;
        }
    }

    bool flag = false;
    if(bFindStart && !bFindEnd && m_count>0)
    {        
        flag = bFindEnd = true;
        i = bytesRead;
        *bEndOfFrame = true;
    }

    if(!bFindStart || !bFindEnd)
    {
        this->close();
        return -1;
    }

    int size = (i<=inBufSize ? i : inBufSize);
    memcpy(inBuf, m_buf, size); 

    if(!flag)
    {
        m_count += 1;
        m_bytesUsed += i;
    }
    else
    {
        m_count = 0;
        m_bytesUsed = 0;
    }

    fseek(m_file, m_bytesUsed, SEEK_SET);
    return size;
}


