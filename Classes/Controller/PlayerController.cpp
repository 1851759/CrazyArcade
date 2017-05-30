#include "PlayerController.h"
#include "Scene/UI/GameMap.h"
#include "api_generated.h"
USING_NS_CC;

bool PlayerController::init()
{
    if (!Layer::init())
    {
        return false;
    }

    addCustomEvent();

    scheduleUpdate();

    return true;
}

PlayerController::~PlayerController()
{

}

Player* PlayerController::createPlayer(const std::string& id, const std::string& role)
{
    auto player = Player::create(id, role);
    if (player)
    {
        this->_playerList.insert(id, player);
        return player;
    }
    return nullptr;
}

Player * PlayerController::createLocalPlayer(const std::string & id, const std::string & role)
{
    localPlayer = createPlayer(id, role);
    if (localPlayer)
    {
        this->_playerList.insert(id, localPlayer);
    }
    return localPlayer;
}

void PlayerController::setStatus(const std::string& id, Player::Status status)
{
    auto player = this->getPlayer(id);
    if (player)
    {
        player->setStatus(status);
    }
}

Player * PlayerController::getPlayer(const std::string& id)
{
    return _playerList.at(id);
}

Player * PlayerController::getLocalPlayer()
{
    return localPlayer;
}

void PlayerController::localPlayerMove()
{
    if (localPlayer->getStatus() == Player::Status::FREE && localPlayer->getDirection() != Player::Direction::NONE)
    {
        auto map = GameMap::getCurrentMap();
        if (map == nullptr) return;

        const auto speed = localPlayer->getSpeed();
        for (uint8_t i = 0; i < speed; ++i)
        {
            auto pair = getNextPos(localPlayer->getPosition(), localPlayer->getDirection());
            auto nextPos = pair.first, logicPos1 = pair.second.first, logicPos2 = pair.second.second;

            if (map->isCanAccess(logicPos1) && map->isCanAccess(logicPos2))
            {
                localPlayer->setPosition(nextPos);
            }
        }
    }
}

void PlayerController::update(float dt)
{
    if (localPlayer) { 
        localPlayerMove();
    }
}

std::pair<cocos2d::Vec2, std::pair<cocos2d::Vec2, cocos2d::Vec2>> PlayerController::getNextPos(const cocos2d::Vec2& pos, Player::Direction direction)
{
    int step = 1;

    Vec2 nextPos(pos.x, pos.y);
    Vec2 logicPos1(pos.x, pos.y), logicPos2(pos.x, pos.y);

    switch (direction)
    {
    case Player::Direction::LEFT:
        nextPos.x -= step;
        logicPos1.x = logicPos2.x = nextPos.x - 20;
        logicPos1.y -= 16;
        logicPos2.y += 16;
        break;
    case Player::Direction::RIGHT:
        nextPos.x += step;
        logicPos1.x = logicPos2.x = nextPos.x + 20;
        logicPos1.y -= 16;
        logicPos2.y += 16;
        break;
    case Player::Direction::UP:
        nextPos.y += step;
        logicPos1.y = logicPos2.y = nextPos.y + 20;
        logicPos1.x += 16;
        logicPos2.x -= 16;
        break;
    case Player::Direction::DOWN:
        nextPos.y -= step;
        logicPos1.y = logicPos2.y = nextPos.y - 20;
        logicPos1.x += 16;
        logicPos2.x -= 16;
        break;
    default:
        break;
    }
    return std::make_pair(nextPos, std::make_pair(logicPos1, logicPos2));
}

void PlayerController::addCustomEvent()
{
    using namespace API;
    auto dispatcher = this->getEventDispatcher();
    dispatcher->addEventListenerWithSceneGraphPriority(EventListenerCustom::create("on_local_player_move", [=](EventCustom* event)
    {
        char* buf = static_cast<char*>(event->getUserData());
        int mode;
        int direction;
        sscanf(buf, "%d %d", &mode, &direction);
        if (mode)
        {
            localPlayer->setDirectionByKey(static_cast<Player::Direction>(direction));
        }
        else
        {
            localPlayer->removeDirectionByKey(static_cast<Player::Direction>(direction));
        }
    }), this);
#ifdef NETWORK  
    dispatcher->addEventListenerWithSceneGraphPriority(EventListenerCustom::create("on_local_player_init", [=](EventCustom* event)
    {
        auto data = static_cast<API::PlayerJoin*>(event->getUserData());
        auto player = createLocalPlayer(data->id()->str());
        auto pos = Vec2(data->x(), data->y());
        player->setPosition(pos);
        GameMap::getCurrentMap()->addChild(player, 1);
    }), this);

    dispatcher->addEventListenerWithSceneGraphPriority(EventListenerCustom::create("on_other_player_move", [=](EventCustom* event)
    {
        auto data = static_cast<API::PlayerPosChange*>(event->getUserData());
        auto player = getPlayer(data->id()->str());
        if (player != localPlayer && player != nullptr)
        {
            auto pos = Vec2(data->x(), data->y());
            auto dir = static_cast<Player::Direction>(data->direction());
            player->setPosition(pos);
            player->setDirection(dir);
        }
    }), this);
#endif
}
