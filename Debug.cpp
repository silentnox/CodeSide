#include "Debug.hpp"
#include "model/PlayerMessageGame.hpp"

Debug::Debug(const std::shared_ptr<OutputStream> &outputStream)
    : outputStream(outputStream) {}

void Debug::draw(const CustomData &customData) {
  outputStream->write(PlayerMessageGame::CustomDataMessage::TAG);
  customData.writeTo(*outputStream);
  outputStream->flush();
}

void Debug::print( const std::string & message )
{
	draw( CustomData::Log( message ) );
}

void Debug::drawLine( Vec2 from, Vec2 to, double width, ColorFloat color )
{
	draw( CustomData::Line( from, to, width, color ) );
}

