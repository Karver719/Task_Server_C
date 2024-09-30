#include <nats/nats.h>
#include <chrono>
#include <iostream>
#include <thread>

void onMsg(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure) {
    // Получаем временную метку из сообщения.
    int64_t t_sign = *(int64_t*)natsMsg_GetData(msg);
    
    // Текущее время.
    auto t_real = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::cout << "Received tick from app1, dt: " << t_real - t_sign << " ms" << std::endl;

    // Отправляем обратно текущее время как ответ.
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
    natsConnection *conn = NULL;
    natsSubscription *sub = NULL;
    natsStatus s = natsConnection_ConnectTo(&conn, NATS_DEFAULT_URL);

    if (s != NATS_OK) {
        std::cerr << "Can't connect: " << natsStatus_GetText(s) << std::endl;
        return 1;
    }

    // Подписываемся на сообщения от первого приложения.
    s = natsConnection_Subscribe(&sub, conn, "time.tick", onMsg, NULL);
    if (s != NATS_OK) {
        std::cerr << "Can't subscribe to time.tick: " << natsStatus_GetText(s) << std::endl;
        return 1;
    }

    // Бесконечный цикл для обработки сообщений.
    while (true) {
        // Здесь ничего не делаем, просто ожидаем сообщения.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    natsSubscription_Destroy(sub);
    natsConnection_Destroy(conn);

    return 0;
}