#include "lottery.hpp"
#include "token/eosio.token.hpp"
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <vector>
using namespace eosio;
using namespace std;

  // right path to eosio.token.hpp file


inline  bool lottery::period_allow(std::string period,ticket *r_ticket){
    allow_idx allow_tbl(get_self(),get_self().value);
    auto allow_itr=allow_tbl.find(stoll(period));
    if(allow_itr==allow_tbl.end())
        return false;
    if(allow_itr->status==0)
        return false;
    return true;
    }
inline  bool lottery::bet_allow(eosio::asset quantity){
    return true;
    }
void lottery::sendtoken(name to ,asset quantity){
    action{
        permission_level{get_self(), "active"_n},
        "eosio.token"_n,
        "transfer"_n,
        std::make_tuple(get_self(),to, quantity, std::string("Congratulations"))
    }.send(); 
    print(quantity);
    }
void lottery::change_seed(const std::vector <int> numbers){
    seed_idx seed_tbl(get_self(),get_self().value);
    auto seed_itr=seed_tbl.find(1);

    checksum256 old_seed=seed_itr->seed;
    auto mixd = old_seed.data()+now()+numbers.at(0)+numbers.at(1)+numbers.at(2)+numbers.at(3)+numbers.at(4);
    const char *mixedChar = reinterpret_cast<const char *>(&mixd);
    checksum256 new_seed;
    new_seed=sha256((char *)mixedChar, sizeof(mixedChar));

    seed_tbl.modify(seed_itr,get_self(),[&](auto &row){
        row.seed=new_seed;
        });

    }
void lottery::string_decode(std::string memo,ticket * r_ticket){
    
    memo.erase(memo.begin(), find_if(memo.begin(), memo.end(), [](int ch) {
            return !isspace(ch);
        }));
    memo.erase(find_if(memo.rbegin(), memo.rend(), [](int ch) {
            return !isspace(ch);
    }).base(), memo.end());

    vector<string> v;
    split(memo, ':', v);
    check(v.size() == 3, "error:memo");
    //bet:xxxxx....
    check(v.at(0)=="bet","error:memo ,with out bet");
    //numbers
    r_ticket->numbers=string_numbers(v.at(1));
    //period
    check(period_allow(v.at(2),r_ticket),"error:period");
    r_ticket->period=v.at(2);

    }
inline vector<int> lottery::string_numbers(string n_s){
    vector<string> s_v;
    split(n_s, ',', s_v);
    check(s_v.size()==5,"error:number of numbrers");
    vector<int> r_v;
    for(int i=0;i<5;i++){
        int t=stoi(s_v.at(i));
        for(int j=0;j<i;j++){
            check(t!=r_v.at(j),"error:same number");
            }
        check(t<=39,"error:number range");
        check(t>=1,"error:number range");
        r_v.push_back(t);
        }
    return r_v;
    }
void lottery::split(const string& s, char c,vector<string>& v) {
    string::size_type i = 0;
    string::size_type j = s.find(c);

    while (j != string::npos) {
        v.push_back(s.substr(i, j-i));
        i = ++j;
        j = s.find(c, j);

        if (j == string::npos)
            v.push_back(s.substr(i, s.length()));
        }
    }
int lottery::open(const vector<int> player_numbers,const vector<int> dealer_numbers,eosio::asset &quantity){
    
    double coe=0;
    int count=0;
    for(int i=0;i<5;i++)
        for(int j=0;j<5;j++)
            if(player_numbers.at(i)==dealer_numbers.at(j))
                count++;

    if(count==0 ||count==1){
        return count;
    }
    else if (count==2){
        coe=PRIZE_4;}
    else if (count==3){
        coe=PRIZE_3;}
    else if (count==4){
        coe=PRIZE_2;}
    else if (count==5){
        coe=PRIZE_1;}
    
    quantity=asset(quantity.amount*coe,symbol(SYMBOL,PRECISION));
    return count;
}
asset lottery::get_balance(name account, symbol symbol)
{
    asset account_balance = token::get_balance("eosio.token"_n, account, symbol.code());
    return account_balance;
}
