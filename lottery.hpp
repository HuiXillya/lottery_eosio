#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <vector>

#define ERR_BOUND 40
#define BET_UPPER_BOUND 100
#define SYMBOL "TNT"
#define PRECISION 4
#define INVESTOR "dealer"
#define SELFNAME "lotery"
#define START_PERIOD 1

#define PRIZE_1 3*160000
#define PRIZE_2 3*400
#define PRIZE_3 3*6
#define PRIZE_4 3

//profit 7.5%

using namespace eosio;
using namespace std;

class [[eosio::contract("lottery")]] lottery : public eosio::contract {
    private:
        const symbol accept_symbol;

        struct [[eosio::table]] ticket {
            uint64_t   serialID;
            eosio::name owner;
            eosio::asset betfunds;
            std::string period;
            std::vector <int> numbers;
            uint64_t primary_key() const { return serialID; }
            uint64_t search_for_name() const { return owner.value; }
            uint64_t search_for_period() const { return std::stoll(period); }
        };
        
        struct [[eosio::table]]allow{
            uint64_t period;
            uint32_t time;
            std::vector <int> numbers;
            std::vector <int> people_win_prize;
            eosio::asset currenpeoplet_balance; 
            uint8_t status;
            //status 0=opened, 1=next open, 2=unopen but allow to bet  
            uint64_t primary_key() const { return period; }
            uint64_t search_for_status() const {return (uint64_t)status;}
        };
        struct seed{
            uint64_t key;
            
            checksum256 seed;
            uint8_t status;
            //status 1 = accept, 0 = reject
            uint64_t primary_key() const { return key; }
        };

        struct [[eosio::table]] target{
            uint64_t period;
            std::vector <unsigned char> numbers;
            uint64_t primary_key() const { return period; }
        };

        
        
        inline bool period_allow(std::string period,ticket *r_ticket);
        //return true if the period is allowed

        inline bool bet_allow(eosio::asset quantity);
        //return true if the bet is allowed
        //maintain upper and lower bound

        void sendtoken(name to ,asset quantity);
        //send token for self

        inline uint32_t now() {
             return current_time_point().sec_since_epoch();}

        void change_seed(const std::vector <int> numbers);
        //change the secret seed

        void split(const string& s, char c,vector<string>& v) ;
        //splits string to vector

        void string_decode(std::string memo,ticket * r_ticket);
        //example formate:"bet:1,2,3,4,5:1"

        inline vector<int> string_numbers(string n_s);
        //check the numbers is league
        //return the vector of numbers splits for string

        int open(const vector<int> player_numbers,const vector<int> dealer_numbers,eosio::asset &quantity);
        //true if player win 
        
        asset get_balance(name account, symbol code);
       
         
    public :
        

        lottery( name receiver, name code, datastream<const char*> ds ):contract(receiver, code, ds),accept_symbol(SYMBOL,PRECISION){}
        //player action
        [[eosio::on_notify("eosio.token::transfer")]]
        void bet(eosio::name user,eosio::name to,eosio::asset quantity,std::string memo);
        [[eosio::action]]
        void redeem(eosio::name user,uint64_t   serialID);
        [[eosio::action]]
        void refund(eosio::name user,uint64_t   serialID);
        [[eosio::action]]
        void redeemall(eosio::name user);

        //dealer action
        [[eosio::action]]
        void setnumbers(eosio::name user);
        [[eosio::action]]
        void initall(eosio::name user);
        [[eosio::action]]
        void shutdown(eosio::name user);
    typedef  eosio::multi_index<"ticket"_n, ticket,
            indexed_by<"byname"_n, const_mem_fun<ticket,uint64_t, &ticket::search_for_name>>,
            indexed_by<"byperiod"_n, const_mem_fun<ticket,uint64_t, &ticket::search_for_period>>> ticket_idx;
    
    typedef eosio::multi_index<"target"_n, target> target_idx;
    typedef eosio::multi_index<"allow"_n, allow,
            indexed_by <"current"_n, const_mem_fun<allow,uint64_t, &allow::search_for_status>>> allow_idx;
    typedef eosio::multi_index<"seed"_n, seed> seed_idx;
    
    };