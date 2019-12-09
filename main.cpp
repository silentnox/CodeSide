#include "Debug.hpp"
#include "MyStrategy.hpp"
#include "TcpStream.hpp"
#include "model/PlayerMessageGame.hpp"
#include "model/ServerMessageGame.hpp"
#include <memory>
#include <string>
#include <unordered_map>

Timer totalTime,stratTime;

class Runner {
public:
  Runner(const std::string &host, int port, const std::string &token) {
    std::shared_ptr<TcpStream> tcpStream(new TcpStream(host, port));
    inputStream = getInputStream(tcpStream);
    outputStream = getOutputStream(tcpStream);
    outputStream->write(token);
    outputStream->flush();
  }
  void run() {
    MyStrategy myStrategy;
    Debug debug(outputStream);
	totalTime.Begin();
    while (true) {
      auto message = ServerMessageGame::readFrom(*inputStream);
      const auto& playerView = message.playerView;
      if (!playerView) {
        break;
      }
      std::unordered_map<int, UnitAction> actions;
      for (const Unit &unit : playerView->game.units) {
        if (unit.playerId == playerView->myId) {
          actions.emplace(std::make_pair(unit.id, myStrategy.getAction(unit, playerView->game, debug)));
        }
      }
      PlayerMessageGame::ActionMessage(actions).writeTo(*outputStream);
      outputStream->flush();
    }
	totalTime.End();
	std::cout << "Total msec: " << totalTime.GetTotalMsec() << std::endl;
  }

private:
  std::shared_ptr<InputStream> inputStream;
  std::shared_ptr<OutputStream> outputStream;
};

int main(int argc, char *argv[]) {
  std::string host = argc < 2 ? "127.0.0.1" : argv[1];
  int port = argc < 3 ? 31001 : atoi(argv[2]);
  std::string token = argc < 4 ? "0000000000000000" : argv[3];
  int seed = argc < 5 ? time( NULL ) : atoi( argv[4] );
  srand( seed );
  Runner(host, port, token).run();
  return 0;
}