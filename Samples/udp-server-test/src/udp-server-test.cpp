
#include <iostream>

#include "netAbstraction/System.h"
#include "netAbstraction/ServerUDP.h"

class ClientHandler
{
public:
    static void initialize(ServerUDP* srv)
    {
        server = srv;
        server->addReceiver(ClientHandler::receivedData);
        server->addConnector(ClientHandler::connectedClient);
        server->addDisconnector(ClientHandler::disconnectedClient);
        server->setHandshake(ClientHandler::handshake);
    }
    static void receivedData(const Layer::Address& from, const std::vector<char>& data)
    {
        std::string str(data.begin(), data.end());
        std::string toSend = names[from.toString()] + " said '" + str + "'";
        std::vector<char> vec(toSend.begin(), toSend.end());
        server->send(vec);
    }
    static void connectedClient(const Layer::Address& from)
    {
        std::string toSend = names[from.toString()] + " just arrived";
        std::vector<char> vec(toSend.begin(), toSend.end());
        server->send(vec);
    }
    static void disconnectedClient(const Layer::Address& from)
    {
        std::string toSend = names[from.toString()] + " just quit";
        std::vector<char> vec(toSend.begin(), toSend.end());
        server->send(vec);
    }
    static bool handshake(const Layer::Address& from, const std::vector<char>& msg)
    {
        std::string uuid = from.toString();
        std::vector<char> vec;
        // receiving name
        std::string name(msg.begin(), msg.end());
        // sending id on server
        vec = std::vector<char>(uuid.begin(), uuid.end());
        server->send(from, vec);
        // registering client
        names[from.toString()] = name;
        return true;
    }
private:
    static ServerUDP* server;
    static std::map<std::string, std::string> names;
};

ServerUDP* ClientHandler::server;
std::map<std::string, std::string> ClientHandler::names;

std::string SelectAdapter()
{
    std::string result = "";
    std::vector<std::string> adapterList = GetAdapterList();
    for (int i = 0; i < adapterList.size(); i++)
    {
        std::cout << i << " :: " << adapterList[i] << std::endl;
    }
    std::cout << "Which interface to use : ";
    std::cout.flush();
    while (true) {
        if (_kbhit()) {
            int ch = _getch() - '0';
            if ((ch >= 0) && (ch < adapterList.size()))
            {
                std::cout << ch << std::endl;
                return adapterList[ch];
            }
        }
    }
    return result;
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
    printf("==============================================================================\n");
    printf("== udp-server-test                                NetEngine::NetAbstraction ==\n");
    printf("==============================================================================\n\n");

    std::string adapter = SelectAdapter();
    int portNumber = SelectPort();

    ServerUDP server(portNumber,adapter);
    ClientHandler::initialize(&server);

    try {

        server.connect();
        server.start();
        std::cout << "UDP Server started" << std::endl;
        while (true)
        {
            if (_kbhit())
            {
                char key = _getch();  // Get the pressed key
                if (key == 'q' || key == 'Q') {
                    std::cout << "\n'Q' key detected. Stopping loop...\n";
                    break;
                }
                if (key == 'l' || key == 'L') {
                    Layer::Address addr;
                    std::vector<char> data;
                    std::cout << "Log::" << std::endl;
                    //LayerUDP::partial_receive(-2, data, addr);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::cout << "stopping server" << std::endl;
        server.stop();
        std::cout << "disconnecting server" << std::endl;
        server.disconnect();
    }
    catch (std::exception e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
