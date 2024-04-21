/**
 * @file debug.c
 * @brief debug模块
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2022-09-03
 * 
 * @copyright Copyright (c) 2022  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */



#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "debug.h"


#define DEBUG_BUF_SIZE 			4096		/* debug缓冲区大小 */
#define DEFAULT_TIMEZONE 		0			/* 时区偏移 */
#define LOG_FILENAME_LEN 		23			/* 日志文件名长度 */

struct LogRecord{
	time_t deadline;			/* 该文件过期时间，过期后需要新建文件 */
	FILE * log_fp;				/* log文件文件描述符 */
};

DBG_LEVEL 					dbg_level = DBG_DEBUG;
static char 				PrintBuf[DEBUG_BUF_SIZE];
static struct LogRecord		logRecord;
static pthread_mutex_t 		debugMutex;

static void PRINT_LOCK(void)
{
	pthread_mutex_lock(&debugMutex);
}

static void PRINT_UNLOCK(void)
{
	pthread_mutex_unlock(&debugMutex);
}

static void dbg_out_str(const char *str)
{
	time_t  c_time = time(NULL);
	
	fputs(str,stdout);
	/* 输出到log中
	*
	* 先检查是否需要新建LOG文件,新建LOG文件后如果数量满足
	* 删除要求，则要删除最早的LOG文件，
	*
	*/
	if(LOG_MAX_FILE_COUNT == 0) 
		return ;

	if(logRecord.deadline < c_time || logRecord.log_fp == NULL)
	{
		char log_name[LOG_FILENAME_LEN + 1];
		char log_pioneer_name[LOG_FILENAME_LEN + 1];
		char log_path[128];
		struct tm time_stamp;
		DIR* dir_ptr;  //the directory
		//struct dirent entry;
		struct dirent *dp;
		int ret,file_count = 0;
		
		if(logRecord.log_fp)
			fclose(logRecord.log_fp);
		
		if((dir_ptr = opendir(LOG_DIR_PATH)) == NULL){
			ret = mkdir(LOG_DIR_PATH,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			if(ret < 0){
				return ;
			}
			dir_ptr = opendir(LOG_DIR_PATH);
			if(dir_ptr == NULL)  return ;
		}

		localtime_r(&c_time, &time_stamp);
		strftime (log_name,sizeof(log_name),"%Y-%m-%d_%H:%M:%S.log",&time_stamp);

		snprintf(log_path,sizeof(log_path),"%s/%s",LOG_DIR_PATH,log_name);
		
		logRecord.log_fp = fopen(log_path,"w");
		if(logRecord.log_fp == NULL){
			closedir(dir_ptr);
			return;
		}
		
		logRecord.deadline = c_time + LOG_INTERVAL_TIME*60;
		strcpy(log_pioneer_name,log_name);
		
		/* 删除旧日志 */
		
		
		while((dp = readdir(dir_ptr))){
			if(dp->d_type == DT_REG){
				if(strlen(dp->d_name) == LOG_FILENAME_LEN && strcmp(dp->d_name,log_name) < 0){
					file_count++;
					if(strcmp(dp->d_name,log_pioneer_name) < 0)
						strcpy(log_pioneer_name,dp->d_name);
				}
			}
		}
		if(file_count >= LOG_MAX_FILE_COUNT){		
			snprintf(log_path,sizeof(log_path),"%s/%s",LOG_DIR_PATH,log_pioneer_name);
			remove(log_path);
		}
		closedir(dir_ptr);
	}
	//fprintf(logRecord.log_fp, "%s",str);
	fputs(str,logRecord.log_fp);
	fflush(logRecord.log_fp);
}

/* 内部使用,未加锁版本 */
static int _debug_nolock_printf(const char *format, ...)
{
	va_list aptr;
	int ret;
	
	va_start(aptr, format);
	ret = vsnprintf(PrintBuf,DEBUG_BUF_SIZE,format, aptr);
	va_end(aptr);
	if(ret > 0)
		dbg_out_str(PrintBuf);
	return ret;
}


void debug_phex(const void *buf,  int len)
{
	const uint8_t *start = buf;
	uint32_t y,x,i=0;
	uint32_t y_n, x_n;
	y_n = (uint32_t)(len / 16);
	x_n = (uint32_t)(len % 16);
	
	PRINT_LOCK();

	_debug_nolock_printf("______________________________________________________________\r\n");
	_debug_nolock_printf("            | 0| 1| 2| 3| 4| 5| 6| 7| 8| 9| A| B| C| D| E| F||\r\n");
	_debug_nolock_printf("--------------------------------------------------------------\r\n");
	
	for(y=0;y  < y_n;y++)
	{
		_debug_nolock_printf("|0x%08x| ", y*16);
		for(x=0;x < 16; x++ )
		{
			_debug_nolock_printf("%02x ",start[i]);
			i++;
		}
		_debug_nolock_printf("|\r\n");
	}
	if(x_n)
	{
		_debug_nolock_printf("|0x%08x| ", y*16);
		for(x=0;x < 16;x++)
		{
			if(x < x_n)
			{
				_debug_nolock_printf("%02x ",start[i]);
				i++;
			}else{
				_debug_nolock_printf("   ");
			}
		}
		_debug_nolock_printf("|\r\n");
	}
	_debug_nolock_printf("--------------------------------------------------------------\r\n");

	PRINT_UNLOCK();
}

/**
	* @brief 打印函数
	* @param  print_time       是否打印时间
	* @param  format           print格式化字符串
	* @param  ...              多参
	* @return int 				返回格式化情况，参考spirntf返回值
	*/
int debug_printf(int is_print_time,const char *format, ...)
{
	va_list aptr;
	int ret;
	time_t Time;
	struct tm Timestamp;
	char str_time[30];
	
	/* 临界区开始 */
	PRINT_LOCK();
	
	va_start(aptr, format);
	ret = vsnprintf(PrintBuf,DEBUG_BUF_SIZE,format, aptr);
	va_end(aptr);
	if(ret > 0)
	{
		if(is_print_time)
		{
			time(&Time);
			Time += DEFAULT_TIMEZONE;
			localtime_r(&Time, &Timestamp);
			strftime (str_time,sizeof(str_time),"[%Y/%m/%d %H:%M:%S] ",&Timestamp);
			dbg_out_str(str_time);
			dbg_out_str(PrintBuf);
		}else{
			dbg_out_str(PrintBuf);
		}
	}
	
	PRINT_UNLOCK();
	return ret;
}

/**
	* @brief debug 模块初始化
	* @return int 
	*/
int debug_init(void)
{
	return pthread_mutex_init(&debugMutex, NULL);
}

/**
	* @brief  debug模块销毁
	*/
void debug_exit(void)
{
	pthread_mutex_destroy(&debugMutex);
}

