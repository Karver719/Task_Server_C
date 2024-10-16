#include <nats/nats.h>
#include <chrono>
#include <iostream>
#include <thread>

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

void onResponseMsg(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure) {
    int64_t t_sign_from_app2 = *(int64_t*)natsMsg_GetData(msg);
    auto t_real = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    std::cout << "Received response from app2, dt: " << t_real - t_sign_from_app2 << " ms" << std::endl;
    natsMsg_Destroy(msg);
}

int main() {
    natsConnection *conn = NULL;
    natsSubscription *sub = NULL;
    natsStatus s = natsConnection_ConnectTo(&conn, NATS_DEFAULT_URL);

    if (s != NATS_OK) {
        std::cerr << "Can't connect: " << natsStatus_GetText(s) << std::endl;
        return 1;
    }

    // Подписываемся на ответы от второго приложения.
    s = natsConnection_Subscribe(&sub, conn, "time.response", onResponseMsg, NULL);
    if (s != NATS_OK) {
        std::cerr << "Can't subscribe to time.response: " << natsStatus_GetText(s) << std::endl;
        return 1;
    }

    const char* subject = "time.tick";

    // Отправляем сообщение каждые 10 мс.
    while (true) {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        int64_t t_sign = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        publishMsg(conn, subject, t_sign);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    natsSubscription_Destroy(sub);
    natsConnection_Destroy(conn);

    return 0;
}