
#include <iostream>

#include "netAbstraction/System.h"
#include "netAbstraction/ClientUDP.h"

class ServerHandler
{
public:
    static void initialize(ClientUDP* cli)
    {
        client = cli;
        client->addReceiver(ServerHandler::receivedData);
        client->setHandshake(ServerHandler::handshake);
    }
    static void receivedData(const std::vector<char>& data)
    {
        std::string str(data.begin(), data.end());
        std::cout << "> " << str << std::endl;
    }
    static bool handshake()
    {
        char name[100];
        std::cout << "handshaking with server" << std::endl;
        std::cout << "What name to use : ";
        std::cin >> name;
        std::string sname(name);
        std::vector<char> vec2(sname.begin(), sname.end());
        client->send(vec2);
        std::vector<char> vec;
        client->receive(vec);
        std::string uuid(vec.begin(), vec.end());
        std::cout << "Client uuid is : " << uuid << std::endl;
        std::cin.getline(name, 100);
        return true;
    }
private:
    static ClientUDP* client;
    static bool first;
};
ClientUDP* ServerHandler::client;
bool ServerHandler::first = true;

std::string SelectIp()
{
    std::string pn;
    std::cout << "Enter server Ip : ";
    std::cout.flush();
    std::cin >> pn;
    return pn;
}

int SelectPort()
{
    int pn;
    std::cout << "Which port to use : ";
    std::cout.flush();
    std::cin >> pn;
    return pn;
}

int main(int argc, char* argv[])
{

    std::string loremIpsum =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
        "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure "
        "dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non "
        "proident, sunt in culpa qui officia deserunt mollit anim id est laborum. "
        "Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Vestibulum tortor quam, feugiat "
        "vitae, ultricies eget, tempor sit amet, ante. Donec eu libero sit amet quam egestas semper. Aenean ultricies mi vitae est. "
        "Mauris placerat eleifend leo. Quisque sit amet est et sapien ullamcorper pharetra. Vestibulum erat wisi, condimentum sed, "
        "commodo vitae, ornare sit amet, wisi. Aenean fermentum, elit eget tincidunt condimentum, eros ipsum rutrum orci, sagittis "
        "tempor lacus enim ac dui. Donec non enim in turpis pulvinar facilisis. Ut felis. Praesent dapibus, neque id cursus faucibus, "
        "tortor neque egestas augue, eu vulputate magna eros eu erat. Aliquam erat volutpat. Nam dui mi, tincidunt quis, accumsan porttitor, "
        "facilisis luctus, metus. ";

    printf("==============================================================================\n");
    printf("== udp-client-test                                NetEngine::NetAbstraction ==\n");
    printf("==============================================================================\n\n");

    std::string ip = SelectIp();
    int port = SelectPort();

    ClientUDP client(ip,port); 
    ServerHandler::initialize(&client);


    try {

        client.connect();
        std::cout << "client connected" << std::endl;
        client.start();
        std::cout << "client started" << std::endl;
        while (true)
        {
            if (_kbhit())
            {
                char key = _getch();  // Get the pressed key
                if (key == 'q' || key == 'Q') {
                    std::cout << "\n'Q' key detected. Stopping loop...\n";
                    break;
                }
                if (key == 's' || key == 'S') {
                    std::cout << "Message : ";
                    char buffer[1024];
                    std::cin.getline(buffer, 1024);
                    std::string sbuffer(buffer);
                    std::vector<char> vec(sbuffer.begin(), sbuffer.end());
                    client.send(vec);
                }
                if (key == 't' || key == 'T') {
                    std::cout << "Sending lorem ipsum" << std::endl;
                    std::vector<char> vec(loremIpsum.begin(), loremIpsum.end());
                    client.send(vec);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::cout << "stopping client" << std::endl;
        client.stop();
        std::cout << "disconnecting client" << std::endl;
        client.disconnect();
    }
    catch (std::exception e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
