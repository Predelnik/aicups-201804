#pragma once

class Response;
class Context;
class GameConfig;

class Strategy {
public:
  virtual Response get_response(const Context &context) = 0;
  virtual void initialize(const GameConfig &config) = 0;
  virtual ~Strategy() = default;
};
