#include "../src/predictor.hpp"
#include <cstdio>
using namespace ibd;
int main(){
  Kin k; k.x=100;k.y=105;k.dx=2;k.dy=0;k.ay=0;k.grounded=true;k.groundY=105;k.mode=Mode::Cube;k.valid=true;
  JumpProfile jp; jp.valid=true; float dy=8; for(int i=0;i<32;i++){ jp.dys.push_back(dy); dy-=0.5f; } // lands after 32 ticks (net 0 -> 8*32-0.5*496=8)
  std::vector<Obstacle> o{{{150,90,180,120},true}};
  for(int d=0; d<=24; d+=4) printf("delay %2d clears=%d\n", d, jumpClears(k,jp,o,d));
  auto c = findCorrection(k,jp,o,6);  printf("maxDelay 6: found=%d | %s\n",c.found,c.reason.c_str());
  auto c3 = findCorrection(k,jp,o,30); printf("maxDelay 30: found=%d delay=%d | %s\n",c3.found,c3.delay,c3.reason.c_str());
}
