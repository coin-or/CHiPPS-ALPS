
#ifndef AlpsException_h_
#define AlpsException_h_

#include <stdlib.h>
#include <string.h>
#include <iostream>

class AlpsException
{
 protected:
  char	file_[1024];
  int   line_;
  char	msg_[1024];

 public:
  AlpsException(const char* file=NULL,
		int line=0,
		const char* msg=NULL ) {
    file_[0] = '\0';
    line_    = line;
    msg_[0]  = '\0';
    
    if(file){
      strncpy(file_, file, sizeof(file_) - 1);
      file_[sizeof(file_) - 1] = '\0';
    }
    if(msg){
      strncpy(msg_, msg, sizeof(msg_) - 1);
      msg_[sizeof(msg_) - 1] = '\0';
    }
  }

  ~AlpsException()		{}
  const char* File() const	{return file_;}
  int	Line() const	        {return line_;}
  const char* Msg() const	{return msg_;}

  friend std::ostream& operator<<(std::ostream& os, const AlpsException& ex);
};

#endif
