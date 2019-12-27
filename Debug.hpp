#ifndef _DEBUG_HPP_
#define _DEBUG_HPP_

#include "Stream.hpp"
#include "model/CustomData.hpp"
#include <memory>

class Debug {
public:
	Debug() {};
  Debug(const std::shared_ptr<OutputStream> &outputStream);
  void draw(const CustomData &customData);

  void flush();

  void print( const std::string & message );
  void drawLine( Vec2 from, Vec2 to, double width = 0.1, ColorFloat color = ColorFloat(1,1,1,1) );
  void drawRect( Rect rect, ColorFloat color );
  void drawWireRect( Rect rect, double width = 0.1, ColorFloat color = ColorFloat(1,1,1,1) );

private:
  std::shared_ptr<OutputStream> outputStream;
};

#endif