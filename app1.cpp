#include <nats/nats.h>
#include <chrono>
#include <iostream>
#include <thread> // Добавлено для std::this_thread

void onMsg(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure) {
    auto t_real = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t t_sign = *(int64_t*)natsMsg_GetData(msg);
    
    std::cout << "dt: " << t_real - t_sign << std::endl;

    int64_t responseTime = t_real;
    natsMsg *response = NULL; 
    natsStatus s = natsMsg_Create(&response, "time.response", NULL, (const char*)&responseTime, sizeof(responseTime));
    if (s == NATS_OK) {
        natsConnection_PublishMsg(nc, response);
        natsMsg_Destroy(response);
    } else {
        std::cerr << "Failed to create response message: " << natsStatus_GetText(s) << std::endl;
    }
    
    natsMsg_Destroy(msg);
}

int main() {
    natsConnection      *conn  = NULL;
    natsStatus          s;

    s = natsConnection_ConnectTo(&conn, NATS_DEFAULT_URL);

    if (s != NATS_OK) {
        std::cerr << "Can't connect: " << natsStatus_GetText(s) << std::endl;
        return 1;
    }

    natsSubscription *sub = NULL;
    s = natsConnection_Subscribe(&sub, conn, "time.tick", onMsg, NULL);

    if (s != NATS_OK) {
        std::cerr << "Can't subscribe: " << natsStatus_GetText(s) << std::endl;
        return 1;
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    natsSubscription_Destroy(sub);
    natsConnection_Destroy(conn);

    return 0;
}