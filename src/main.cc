#include "../include/web_server.h"
#include "../include/threadpool.h"


int main(){
    Config config;
    config.get_config();
    /* config.Close(); */
    if(config.Daemon() == 1){
        daemon(1,0);
    }
    Listen_Sock listen_sock;
    listen_sock.Init(config);
    pool_init(config.Max_thread());

    for(;;){
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int client_sock = accept(listen_sock.listen_sock(),(struct sockaddr*)&client,&len);
        if(client_sock < 0){
            continue;
        }
        pool_add_work(server_handle,(void*)&client_sock);
        /* pthread_t pid; */
        /* pthread_create(&pid,NULL,server_handle,(void*)&client_sock); */
        /* pthread_detach(pid); */
    }
    return 0;
}
