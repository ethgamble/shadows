#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>

#define GAME_CONTRACT N(eosioshadows)

using namespace eosio;
using namespace std;

class shadows : public eosio::contract {
public:
    const uint64_t INIT_TIME = 1534075688; // 2018-08-12 20:08:08 启动游戏

    shadows(account_name self):eosio::contract(self), _global(self, self), users(GAME_CONTRACT, GAME_CONTRACT),games(GAME_CONTRACT, GAME_CONTRACT){
        auto gl_itr = _global.begin();
        if (gl_itr == _global.end())
        {
            gl_itr = _global.emplace(_self, [&](auto &gl) {
                gl.owner = _self;
                gl.next_id = 0x0fffffffffffffff;
            });
        }
    }

    /// @abi action 
    void buy(asset quant){
        auto gl_itr = _global.begin();
        require_auth(gl_itr->owner);
        auto useritr = users.find( _self );
        if( now() < INIT_TIME && useritr == users.end()){
            transaction out; //构造交易
            out.actions.emplace_back(permission_level{_self, N(active)}, _self, N(buy), make_tuple(quant)); //将指定行为绑定到该交易上
            out.delay_sec = 1; //设置延迟时间，单位为1秒
            out.send(_next_id(), _self, false); //发送交易，第一个参数为该次交易发送id，每次需不同。如果两个发送id相同，则视第三个参数replace_existing来定是覆盖还是直接失败。
        }else{
            if(useritr == users.end()){
                action(
                    permission_level{_self, N(active)},
                    N(eosio.token), N(transfer),
                    make_tuple(_self, GAME_CONTRACT, quant, string("buy")))
                .send();
            }
        }
    }

    /// @abi action 
    void sell(){
        auto gl_itr = _global.begin();
        require_auth(gl_itr->owner);
        auto useritr = users.find( _self );
        if(useritr != users.end()){
            if(useritr->k <= 1000000*10000ll){
                action(
                    permission_level{_self, N(active)},
                    GAME_CONTRACT, N(sell),
                    make_tuple(_self, asset(useritr->k, S(4, SHARE))))
                .send();
            } else {
                action(
                    permission_level{_self, N(active)},
                    GAME_CONTRACT, N(sell),
                    make_tuple(_self, asset(1000000, S(4, SHARE))))
                .send();
            }
        }
    }

    uint64_t _next_id(){
        auto gl_itr = _global.begin();
        _global.modify(gl_itr, 0, [&](auto &gl) {
            gl.next_id++;
        });
        return gl_itr->next_id;
    }

private:
    // @abi table global i64
    struct global{
        account_name owner;
        uint64_t next_id;
        uint64_t primary_key() const { return owner; }
        EOSLIB_SERIALIZE(global, (owner)(next_id))
    };

    typedef eosio::multi_index<N(global), global> global_index;
    global_index _global;

    // @abi table games i64
    struct game{
      uint64_t i;
      uint64_t k;
      uint64_t e;
      uint64_t f;
      uint64_t w;
      uint64_t r;
      uint64_t d;

      uint64_t primary_key() const { return i; }
      EOSLIB_SERIALIZE(game, (i)(k)(e)(f)(w)(r)(d))
    };
    typedef eosio::multi_index<N(games), game> game_list;
    game_list games;
    
    // @abi table users i64
    struct user {
      account_name n;
      account_name r;
      uint64_t e;
      uint64_t k;
      int64_t p;
      uint64_t t;

      uint64_t primary_key() const { return n; }
      uint64_t get_key() const { return k; }
      EOSLIB_SERIALIZE(user, (n)(r)(e)(k)(p)(t))
    };
    typedef eosio::multi_index<N(users), user,
    indexed_by<N(k), const_mem_fun<user, uint64_t, &user::get_key>>
    > user_list;
    user_list users;

};

 #define EOSIO_ABI_EX( TYPE, MEMBERS ) \
 extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
       if( action == N(onerror)) { \
          eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account"); \
       } \
       auto self = receiver; \
       if(code == self && (action==N(buy) || action==N(sell) || action == N(onerror))) { \
          TYPE thiscontract( self ); \
          switch( action ) { \
             EOSIO_API( TYPE, MEMBERS ) \
          } \
       } \
    } \
 }

EOSIO_ABI_EX(shadows, (buy)(sell))