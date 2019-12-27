#include "Debug.hpp"
#include "model/PlayerMessageGame.hpp"

Debug::Debug(const std::shared_ptr<OutputStream> &outputStream)
    : outputStream(outputStream) {}

void Debug::draw(const CustomData &customData) {
  outputStream->write(PlayerMessageGame::CustomDataMessage::TAG);
  customData.writeTo(*outputStream);
  //outputStream->flush();
}

void Debug::flush()
{
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

void Debug::drawRect( Rect rect, ColorFloat color )
{
	draw( CustomData::Rect( rect.Min, rect.Size(), color ) );
}

void Debug::drawWireRect( Rect rect, double width, ColorFloat color )
{
	drawLine( rect.Min, Vec2( rect.Min.x, rect.Max.y ), width, color );
	drawLine( rect.Min, Vec2( rect.Max.x, rect.Min.y ), width, color );
	drawLine( rect.Max, Vec2( rect.Min.x, rect.Max.y ), width, color );
	drawLine( rect.Max, Vec2( rect.Max.x, rect.Min.y ), width, color );
}

