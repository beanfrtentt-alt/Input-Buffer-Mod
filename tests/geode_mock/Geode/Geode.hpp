#pragma once
// MINIMAL MOCK of the Geode API surface used by main.cpp - for compile-checking my own C++ only.
#include <format>
#include <vector>
#include <string>
#include <functional>
#include <filesystem>
#include <cstdint>
#include <cmath>
#include <algorithm>
namespace fmt { using std::format; }
#define $modify(N, B) N : public B
namespace cocos2d {
 struct CCPoint { float x=0,y=0; CCPoint()=default; CCPoint(float a,float b):x(a),y(b){} };
 struct CCSize { float width=0,height=0; };
 struct CCRect { CCPoint origin; CCSize size; };
 struct ccColor3B { unsigned char r,g,b; };
 struct ccColor4B { unsigned char r,g,b,a; };
 struct ccColor4F { float r,g,b,a; };
 enum enumKeyCodes { KEY_None=0, KEY_F6=0x75, KEY_F8=0x77 };
}
struct KeyboardInputData { enum class Action { Press, Release, Repeat }; cocos2d::enumKeyCodes key; Action action; };
struct KeyboardInputEvent { KeyboardInputEvent(){} };
namespace geode { namespace prelude {} }
using namespace cocos2d;
struct CCAction {};
struct CCFiniteTimeAction : CCAction {};
struct CCNode {
  virtual ~CCNode()=default;
  void addChild(CCNode*){} void setZOrder(int){} void setPosition(CCPoint){} void setVisible(bool){}
  void setScale(float){} void setOpacity(unsigned char){} void setAnchorPoint(CCPoint){} void setColor(ccColor3B){}
  void runAction(CCAction*){} float getPositionX(){return 0;} float getPositionY(){return 0;}
  template<class F> void addEventListener(KeyboardInputEvent, F){}
};
struct CCLayer : CCNode {};
struct CCLabelBMFont : CCNode { static CCLabelBMFont* create(const char*,const char*){return new CCLabelBMFont;} void setString(const char*){} };
struct CCLayerColor : CCNode { static CCLayerColor* create(ccColor4B,float,float){return new CCLayerColor;} };
struct CCDrawNode : CCNode { static CCDrawNode* create(){return new CCDrawNode;} void clear(){}
  void drawSegment(const CCPoint&,const CCPoint&,float,const ccColor4F&){} void drawDot(const CCPoint&,float,const ccColor4F&){} };
struct CCFadeTo : CCFiniteTimeAction { static CCFadeTo* create(float,unsigned char){return new CCFadeTo;} };
struct CCRemoveSelf : CCFiniteTimeAction { static CCRemoveSelf* create(){return new CCRemoveSelf;} };
struct CCSequence : CCAction { template<class...A> static CCSequence* create(A...){return new CCSequence;} };
struct CCDirector { static CCDirector* get(){static CCDirector d; return &d;} CCSize getWinSize(){return {570,320};} };
enum class GameObjectType { Solid=0, Hazard=2, AnimatedHazard=47 };
struct GameObject { bool m_isDisabled=false, m_isTrigger=false; GameObjectType m_objectType=GameObjectType::Solid;
  CCRect rc; CCRect const& getObjectRect(){return rc;} };
struct PlayerObject : CCNode { bool m_isShip,m_isBall,m_isBird,m_isDart,m_isRobot,m_isSpider,m_isSwing,m_isUpsideDown,m_isOnGround,m_isDead; float m_playerSpeed; };
struct GJGameLevel {};
struct GJBaseGameLayer : CCLayer {
  PlayerObject *m_player1=nullptr,*m_player2=nullptr;
  std::vector<std::vector<std::vector<GameObject*>*>*> m_sections; int m_leftSectionIndex=0,m_rightSectionIndex=0;
  virtual void handleButton(bool,int,bool){} virtual void processCommands(float,bool,bool){}
};
struct PlayLayer : GJBaseGameLayer {
  CCNode *m_uiLayer=nullptr,*m_objectLayer=nullptr;
  static PlayLayer* get(){return nullptr;}
  virtual bool init(GJGameLevel*,bool,bool){return true;} virtual void postUpdate(float){}
  virtual void destroyPlayer(PlayerObject*,GameObject*){} virtual void resetLevel(){} void levelComplete(){} void onQuit(){}
};
struct Mod { static Mod* get(){static Mod m; return &m;}
  template<class T> T getSettingValue(const char*){return T{};} std::filesystem::path getSaveDir(){return ".";} };
namespace geode { namespace log { template<class...A> void info(const char*, A&&...){} } }
using namespace geode;
