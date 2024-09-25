#include <nats/nats.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdio>

void publishMsg(natsConnection *conn, const char* subject, int64_t t_sign) {
    natsMsg *msg = NULL; 
    natsStatus s = natsMsg_Create(&msg, subject, NULL, (const char*)&t_sign, sizeof(t_sign));
    if (s == NATS_OK) {
        natsConnection_PublishMsg(conn, msg);
        natsMsg_Destroy(msg);
    } else {
        std::cerr << "Failed to create message: " << natsStatus_GetText(s) << std::endl;
    }
}

int main() {
    natsConnection      *conn  = NULL;
    natsOptions         *opts  = NULL;
    natsStatus          s;

    s = natsOptions_Create(&opts);
    if (s == NATS_OK)
        s = natsOptions_SetURL(opts, "nats://localhost:4222"); 

    if (s == NATS_OK)
        s = natsConnection_Connect(&conn, opts);

    if (s != NATS_OK) {
        std::cerr << "Can't connect: " << natsStatus_GetText(s) << std::endl;
        return 1;
    }

    const char* subject = "time.tick";

    while (true) {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        int64_t t_sign = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        publishMsg(conn, subject, t_sign);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        natsSubscription *sub = NULL;
        s = natsConnection_SubscribeSync(&sub, conn, "time.response");
        if (s == NATS_OK) {
            natsMsg *msg = NULL;
            s = natsSubscription_NextMsg(&msg, sub, 1000);
            if (s == NATS_OK) {
                int64_t t_sign_from_app2 = *(int64_t*)natsMsg_GetData(msg);
                auto t_real = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                std::cout << "dt from app1: " << t_real - t_sign_from_app2 << std::endl;
                natsMsg_Destroy(msg);
            }
            natsSubscription_Destroy(sub);
        }
    }

    natsConnection_Destroy(conn);
    natsOptions_Destroy(opts);

    return 0;
}