#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>


//Attantion!!! libuv v 1.0 is used!!!

uv_tcp_t server;
uv_loop_t *loop;


void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf){
	uv_write_t *req=(uv_write_t *)malloc(sizeof(uv_write_t));
	memset(req,0,sizeof(uv_write_t));
	uv_write(req, stream, buf,1,NULL);
	free(buf->base);
	free(req);
}


//uv_buf_t alloc_buff_cb(uv_handle_t *handle,size_t size){

void alloc_buff_cb(uv_handle_t* handle, size_t s_size, uv_buf_t* buf){
	buf->base=malloc(s_size);
	buf->len=s_size;
	memset(buf->base,0,buf->len);
}



void conn_cb(uv_stream_t *srv,int status){

	uv_tcp_t *client =malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop,client);
	uv_accept(srv,(uv_stream_t *) client);
	uv_read_start((uv_stream_t *)client,alloc_buff_cb,read_cb);

}


int main(int argc,char ** argv){
	loop=uv_default_loop();
	struct sockaddr_in addr;

	uv_ip4_addr("0.0.0.0",12345,&addr);

	uv_tcp_init(loop,&server);

	uv_tcp_bind(&server,(struct sockaddr *)&addr,0);
	
	uv_listen((uv_stream_t *)&server,128,conn_cb);

	return uv_run(loop,UV_RUN_DEFAULT);
}