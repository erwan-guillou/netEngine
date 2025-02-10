
#include <iostream>

#include "netAbstraction/System.h"
#include "netAbstraction/ClientTCP.h"

class ServerHandler
{
public:
    static void initialize(ClientTCP* cli)
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
    static ClientTCP* client;
};
ClientTCP* ServerHandler::client;

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
        "facilisis luctus, metus. "
        "Curabitur eros dui, suscipit et dui vel, tincidunt dignissim justo. Integer luctus nulla at turpis rhoncus, ac porta lorem "
        "porttitor. Aenean pulvinar libero nec quam vestibulum, a fringilla risus ultricies. Donec in faucibus nisi, sed sollicitudin "
        "tellus. Cras tincidunt sapien id augue consectetur, vitae tristique ligula viverra. Integer varius lectus ut lectus malesuada, "
        "nec eleifend eros sagittis. Donec ut justo justo. Aenean molestie lectus id lorem cursus, et dictum metus pellentesque. "
        "Suspendisse potenti. Mauris pharetra justo id ligula hendrerit, id dapibus neque iaculis. "
        "Aliquam erat volutpat. Nullam facilisis orci nec eros sagittis, id interdum libero auctor. Curabitur at urna at erat egestas "
        "accumsan. Donec non justo vitae erat cursus convallis. Nunc consectetur, tortor at gravida eleifend, orci eros rhoncus ex, "
        "a laoreet nisi ligula eget nunc. Nulla facilisi. Sed consectetur hendrerit orci vel euismod. Morbi quis metus sapien. "
        "Nullam convallis turpis quis justo gravida, ut volutpat lectus dignissim. "
        "Suspendisse suscipit nunc nec nulla ornare feugiat. Maecenas bibendum ultricies bibendum. Vestibulum non sapien suscipit, "
        "mollis justo at, tristique orci. Nam cursus nisi a dolor tincidunt, non malesuada odio auctor. Sed vitae scelerisque risus. "
        "Quisque varius elit quis nunc malesuada, sed fermentum augue sodales. Vestibulum a semper lacus. Sed hendrerit risus vitae "
        "ultrices dictum. Duis nec sapien mi. "
        "Nunc auctor augue id elit convallis, ut vehicula nulla facilisis. Proin dictum magna in sapien auctor, in dictum sapien sodales. "
        "Etiam id varius risus, vel ultrices dui. Integer tincidunt est a arcu blandit, in interdum elit feugiat. Fusce consequat sapien "
        "lorem, id euismod ligula ullamcorper nec. Aliquam erat volutpat. Nulla eu risus diam. Vestibulum ante ipsum primis in faucibus "
        "orci luctus et ultrices posuere cubilia curae; Quisque tincidunt, justo non varius dapibus, magna ligula scelerisque elit, non "
        "faucibus felis ipsum non erat. Donec ac varius erat. Vivamus ac risus at urna pharetra gravida id id nulla. "
        "Sed scelerisque risus nisi, id volutpat ligula tincidunt nec. Aenean at sagittis lacus. Pellentesque volutpat risus id "
        "nibh posuere, non auctor dui cursus. Cras egestas leo in urna aliquet, ut efficitur mauris tristique. Ut euismod erat sed "
        "nisl lobortis, ut vehicula ligula fringilla. Curabitur lacinia dolor id lectus fringilla volutpat. Aliquam erat volutpat. "
        "Vestibulum id convallis nulla, nec vehicula libero. Suspendisse potenti. Vivamus ut dapibus purus. "
        "Morbi sagittis justo non eros pharetra vehicula. Integer sit amet augue sit amet turpis mollis dapibus. Cras rhoncus dui ut "
        "turpis suscipit, et gravida mi consequat. Nullam convallis mauris at nisl suscipit, eget auctor arcu sagittis. Praesent a "
        "diam sed nunc sagittis fermentum. Donec a sem convallis, tempor mi in, sodales elit. Nunc facilisis eu lectus nec imperdiet. "
        "Cras non leo et justo gravida faucibus. Nullam vel tincidunt nisi, a hendrerit felis. Suspendisse in sapien nec eros suscipit "
        "dignissim. "
        "Etiam lobortis mauris at sapien elementum, vel lacinia neque pharetra. Donec vitae varius magna. Mauris pharetra nulla et "
        "libero interdum, eget venenatis est egestas. Pellentesque nec ante elit. Sed vulputate, nisl sed hendrerit tincidunt, turpis "
        "turpis bibendum ex, sit amet sollicitudin lectus turpis ac tortor. Nullam id diam vel arcu pharetra facilisis et ut erat. "
        "Proin egestas, mauris id efficitur venenatis, dolor justo gravida quam, eget consectetur metus erat eu ligula. Nam vestibulum "
        "nunc quis tincidunt dignissim. Maecenas nec lacinia lectus, id varius nisl. "
        "Mauris ornare vestibulum diam, ac scelerisque odio malesuada ac. Duis fermentum turpis et mi elementum, vel tincidunt ligula "
        "commodo. Quisque lacinia sapien et massa aliquet, eget luctus libero dictum. In sodales sapien eget justo ullamcorper, vel "
        "efficitur odio sagittis. Donec egestas felis non magna vulputate, sed tristique velit aliquam. Vestibulum tempus risus sed "
        "dolor consectetur venenatis. Vestibulum suscipit, lacus a ultricies euismod, dolor mi luctus velit, ac cursus risus ipsum et "
        "lorem. Suspendisse potenti.";

    printf("==============================================================================\n");
    printf("== tcp-client-test                                NetEngine::NetAbstraction ==\n");
    printf("==============================================================================\n\n");

    std::string ip = SelectIp();
    int port = SelectPort();

    ClientTCP client(ip, port);
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
                    char message[1024];
                    std::cout << "What message to send : ";
                    std::cin.getline(message, 1024);
                    std::string sname(message);
                    std::vector<char> vec2(sname.begin(), sname.end());
                    client.send(vec2);
                }
                if (key == 't' || key == 'T') {
                    std::cout << "Sending lorem ipsum" << std::endl;
                    std::vector<char> vec(loremIpsum.begin(), loremIpsum.end());
                    client.send(vec);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::cout << "disconnecting client" << std::endl;
        client.disconnect();
        std::cout << "stopping client" << std::endl;
        client.stop();
    }
    catch (std::exception e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
