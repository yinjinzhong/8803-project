#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>


#include <pthread.h>
#include <sys/times.h>
#include <sys/select.h>
#include <assert.h>
#include <sys/types.h>
#include <cutils/atomic.h>   
#include <hardware/hardware.h>  

#include "serial.h"


//串口相关
struct termios serial_oldtio;
struct termios serial_newtio;
fd_set read_fds;
	
 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 static int set_serial_opts(int fd, int nSpeed, int nBits, char nEvent, int nStop)
 {
	int ret = -1;
	
	if (fd < 0)
		return -1;
	
	//将目前终端机参数保存到结构体变量flydvd_oldtio中
	if (tcgetattr(fd, &serial_oldtio) != 0)
	{
		goto fail;
	}
	
	//结构体serial_newtio清0
	memset(&serial_newtio, 0, sizeof(serial_newtio));
			
	//设置串口参数
	serial_newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	serial_newtio.c_oflag &= ~OPOST;
	serial_newtio.c_iflag &= ~(ICRNL | IGNCR);
	serial_newtio.c_cflag |= CLOCAL | CREAD;           //启动接收器
	serial_newtio.c_cflag &= ~CSIZE;

	serial_newtio.c_cc[VTIME] = 0; 
	serial_newtio.c_cc[VMIN]  = 0; 
	
	serial_newtio.c_cc[VINTR] = 0; 
	serial_newtio.c_cc[VQUIT] = 0; 
	serial_newtio.c_cc[VERASE] = 0; 
	serial_newtio.c_cc[VKILL] = 0; 
	serial_newtio.c_cc[VEOF] = 0; 
	serial_newtio.c_cc[VTIME] = 1; 
	serial_newtio.c_cc[VMIN] = 0; 
	serial_newtio.c_cc[VSWTC] = 0; 
	serial_newtio.c_cc[VSTART] = 0; 
	serial_newtio.c_cc[VSTOP] = 0; 
	serial_newtio.c_cc[VSUSP] = 0; 
	serial_newtio.c_cc[VEOL] = 0; 
	serial_newtio.c_cc[VREPRINT] = 0; 
	serial_newtio.c_cc[VDISCARD] = 0; 
	serial_newtio.c_cc[VWERASE] = 0; 
	serial_newtio.c_cc[VLNEXT] = 0; 
	serial_newtio.c_cc[VEOL2] = 0; 


	
	switch (nBits)
	{
		case 7: serial_newtio.c_cflag |= CS7;break;
		case 8: serial_newtio.c_cflag |= CS8;break;
		default:
			serial_newtio.c_cflag |= CS8;break;
	}
	
	
	switch (nEvent)
	{
		case 'O':
			serial_newtio.c_cflag |= PARENB;
			serial_newtio.c_cflag |= PARODD;
			serial_newtio.c_iflag |= (INPCK|ISTRIP);
			break;
			
		case 'E':
			serial_newtio.c_cflag |= PARENB;
			serial_newtio.c_cflag &= ~PARODD;
			serial_newtio.c_iflag |= (INPCK|ISTRIP);
			break;
		
		case 'N':
			serial_newtio.c_cflag &= ~PARENB;
			break;
		
		default:
			serial_newtio.c_cflag &= ~PARENB;
			break;
	}
	
	switch (nSpeed)
	{
		case 9600:
			cfsetispeed(&serial_newtio, B9600);
			cfsetospeed(&serial_newtio, B9600);
			break;
		
		case 19200:
			cfsetispeed(&serial_newtio, B19200);
			cfsetospeed(&serial_newtio, B19200);
			break;
			
		case 38400:
			cfsetispeed(&serial_newtio, B38400);
			cfsetospeed(&serial_newtio, B38400);
			break;
		
		case 57600:
			cfsetispeed(&serial_newtio, B57600);
			cfsetospeed(&serial_newtio, B57600);
			break;
			
		case 115200:
			cfsetispeed(&serial_newtio, B115200);
			cfsetospeed(&serial_newtio, B115200);
			break;
			
		default:
			cfsetispeed(&serial_newtio, B9600);
			cfsetospeed(&serial_newtio, B9600);
			break;
	}
	
	if (nStop == 1)
		serial_newtio.c_cflag &= ~CSTOPB;
	else if (nStop == 2)
		serial_newtio.c_cflag = CSTOPB;
		
	//刷新在串口中的输入输出数据
	tcflush(fd, TCIFLUSH);
			
	//设置当前的的串口参数为flydvd_newtio
	if (tcsetattr(fd, TCSANOW, &serial_newtio) != 0)
	{
		goto fail;
	}
	
	ret = 0;
			
	fail:
		return ret;
 }
 
 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 int serial_write(int fd, unsigned char *buf, int len)
 {
	int ret = -1;
	
	if (fd < 0)
		return ret;
		
	//写入数据
	ret = write(fd, buf, len);
	
	return ret;
 }
 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 long serial_read(int fd, unsigned char *buf, int len)
 {
	long ret = -1;
	struct timeval tv_timeout;

	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);
	tv_timeout.tv_sec = 1;
	tv_timeout.tv_usec = 0;

	switch (select(fd+1, &read_fds, NULL, NULL, &tv_timeout))
	{
		case -1:
			//DLOGE("select error");
			break;
				
		case 0:
			//DLOGE("select timeout");
			break;
				
		default:
			if (1)
			//if (FD_ISSET(fd, &read_fds))
			{
				//读取数据
				ret = read(fd, buf, len);
			}
			break;	
		}
		
	return ret;
 }
 
 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
int serial_open(void)
{
	int fd = -1;
	int res = 0;
	pthread_t thread_id;
	
	fd = open(SERIAL_NAME,O_RDWR | O_NOCTTY | O_NDELAY);
	//fd = open("/dev/ttyS0",O_RDWR | O_NOCTTY | O_NDELAY);
	//fd = open("/dev/ttyS0",O_RDWR 
	//				| O_NOCTTY  | O_NDELAY | O_NONBLOCK);
	if (fd > 0)     
	{  
		if (set_serial_opts(fd, SERIAL_BAUDRATE, 8, 'N', 1) == -1)
		{
			return -1;
		}
		
		FD_ZERO(&read_fds);
	}

	return fd;
}
 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
int serial_close(int fd)
{
	if (fd < 0)
		return -1;
		
	//恢复旧的端口参数
	tcsetattr(fd, TCSANOW, &serial_oldtio);
		
	//关闭串口
	if (!close(fd))
	{
		return 0;
	}	
	
	return -1;
}