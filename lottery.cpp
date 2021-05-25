
#include "lottery_private.cpp"
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <vector>
#include <string>

using namespace eosio;
using namespace std;

/*memo

get_first_receiver() or  getself()?

shut down
*/

void lottery::bet(eosio::name user,eosio::name to,eosio::asset quantity,std::string memo){
    if(to!=get_self())
        return;
    ticket_idx ticket_tbl(get_self(), get_self().value);
    ticket tmp_ticket;
    tmp_ticket.owner=user;
    check(bet_allow(quantity),"error:symbol");
    tmp_ticket.betfunds=quantity;
    string_decode(memo,&tmp_ticket);
    auto apk=ticket_tbl.available_primary_key();
    ticket_tbl.emplace(get_self(),[&](auto& row){
        row=tmp_ticket;
        row.serialID=apk;
    });
    print("your ticket ID is : ");
    print(apk);
    print("  \n Please rember it");
}
void lottery::redeem(eosio::name user,uint64_t   serialID){
    //to do check user identity
    require_auth(user);
    ticket_idx ticket_tbl(get_self(), get_self().value);
    auto ticket_itr=ticket_tbl.find(serialID);

    check(ticket_itr!=ticket_tbl.end(),"error:serialID");
    check(user==ticket_itr->owner,"error:user");

    allow_idx allow_tbl(get_self(),get_self().value);
    auto allow_itr=allow_tbl.find(stoll(ticket_itr->period));

    check(allow_itr->status==0,"error:period");

    eosio::asset quantity=ticket_itr->betfunds;

    if(open(ticket_itr->numbers,allow_itr->numbers,quantity)>1){
        sendtoken(user,quantity);
    }
    ticket_tbl.erase(ticket_itr);
}
void lottery::refund(eosio::name user,uint64_t   serialID){
    require_auth(user);
    ticket_idx ticket_tbl(get_self(), get_self().value);
    auto ticket_itr=ticket_tbl.find(serialID);

    check(ticket_itr!=ticket_tbl.end(),"error:serialID");
    check(user==ticket_itr->owner,"error:user");

    allow_idx allow_tbl(get_self(),get_self().value);
    auto allow_itr=allow_tbl.find(stoll(ticket_itr->period));

    check(allow_itr->status!=0,"error:period");

    eosio::asset quantity=ticket_itr->betfunds;
    sendtoken(user,quantity);

    ticket_tbl.erase(ticket_itr);


}
void lottery::redeemall(eosio::name user){
    require_auth(user);
    ticket_idx ticket_tbl(get_self(), get_self().value);
    allow_idx allow_tbl(get_self(),get_self().value);
    auto allow_itr=allow_tbl.begin();
    auto ticket_itr=ticket_tbl.begin();
    eosio::asset quantity;
    while(ticket_itr!=ticket_tbl.end()){
        
        if(ticket_itr->owner!=user){
            ticket_itr++;
            continue;
        }
        allow_itr=allow_tbl.find(stoll(ticket_itr->period));
        auto next_ticket_itr=ticket_itr;
        next_ticket_itr++;

        if(allow_itr->status==0){
            
            quantity=ticket_itr->betfunds;
            if(open(ticket_itr->numbers,allow_itr->numbers,quantity)>1){
                sendtoken(user,quantity);
                }
            ticket_tbl.erase(ticket_itr);
            }
        ticket_itr=next_ticket_itr;
    }

}

void lottery::initall(eosio::name user){
    require_auth(name(INVESTOR));
    allow_idx allow_table(get_self(),get_self().value);

    allow_table.emplace(get_self(),[&](auto& row){
        row.period=std::stoull(std::to_string(START_PERIOD));
        row.status=1;
    });

    for(int i=1;i<100;i++){
        allow_table.emplace(get_self(),[&](auto& row){
            row.period=std::stoull(std::to_string(START_PERIOD+i));
            row.status=2;
        });
    }
    auto mixd =now();
    const char * mixedChar = reinterpret_cast<const char *>(&mixd);
    seed_idx seed_tbl(get_self(),get_self().value);
    seed_tbl.emplace(get_self(),[&](auto& row){
        row.key=1;
        row.seed=sha256((char *)mixedChar, sizeof(mixedChar));
        row.status=1;
    });
    


}

void lottery::shutdown(eosio::name user){
    require_auth(name(INVESTOR));

    allow_idx allow_tbl(get_self(),get_self().value);
    auto allow_itr =allow_tbl.begin();


    ticket_idx ticket_tbl(get_self(),get_self().value);
    auto ticket_itr=ticket_tbl.begin();
    auto next_ticket_itr=ticket_itr;

    auto sb=get_balance(get_self(),accept_symbol);

    eosio::asset quantity;
    while(ticket_itr!=ticket_tbl.end()){
        next_ticket_itr++;
        allow_itr=allow_tbl.find(stoll(ticket_itr->period));
        if(allow_itr->status==0){
            
            quantity=ticket_itr->betfunds;
            if(open(ticket_itr->numbers,allow_itr->numbers,quantity)>1){
                sendtoken(user,quantity);
                sb.amount-=quantity.amount;
                }
            ticket_tbl.erase(ticket_itr);
            ticket_itr=ticket_tbl.begin();
            }
        else{
             sb.amount-=ticket_itr->betfunds.amount;
            sendtoken(ticket_itr->owner,ticket_itr->betfunds);
            ticket_tbl.erase(ticket_itr);
            ticket_itr = next_ticket_itr;
            }
    }

    seed_idx seed_tbl(get_self(),get_self().value);
    seed_tbl.modify(seed_tbl.begin(),get_self(),[&](auto &row){
                row.status=0;
            });
    sendtoken(name(INVESTOR),sb);
    //transform alltoken to INVESTOR
    return;
}
void lottery::stopbet(eosio::name user){
    require_auth(name(INVESTOR));

    allow_idx allow_tbl(get_self(),get_self().value);
    auto allow_itr=allow_tbl.begin();

    while(allow_itr!=allow_tbl.end()){
        if(allow_itr->status==1){
            break;
        }
        else{
            allow_itr++;
        }
    }
    check(allow_itr!=allow_tbl.end(),"error:period dosent exist");

    allow_tbl.modify(allow_itr,get_self(),[&](auto &row){

                row.status=4;
                row.stoptime=now();
        });
}
void lottery::setnumbers(eosio::name user){
    //check user
    //check period
    require_auth(name(INVESTOR));

    allow_idx allow_tbl(get_self(),get_self().value);
    auto allow_itr=allow_tbl.begin();

    while(allow_itr!=allow_tbl.end()){
        if(allow_itr->status==4){
            break;
        }
        else{
            allow_itr++;
        }
    }


    check(allow_itr!=allow_tbl.end(),"error:period dosent exist");
    //check(allow_itr->status==1,"error:period ");
    auto period =std::to_string(allow_itr->period);

    seed_idx seed_tbl(get_self(),get_self().value);
    auto seed_itr=seed_tbl.find(1);

    checksum256 old_seed=seed_itr->seed;
    auto mixd = old_seed.data()+now();
    const char * mixedChar = reinterpret_cast<const char *>(&mixd);
    checksum256 new_seed;
    new_seed=sha256((char *)mixedChar, sizeof(mixedChar));

    ticket_idx ticket_tbl(get_self(),get_self().value);
    auto ticket_itr=ticket_tbl.begin();
    int lcount=0;
    uint64_t mix_balance;
    while (ticket_itr!=ticket_tbl.end())
    {

        if(ticket_itr->period==period){
            lcount++;
            mix_balance+=get_balance(ticket_itr->owner,accept_symbol).amount;
        }
        if(lcount==30){
            auto mixd2=new_seed.data()+mix_balance;
            mix_balance=get_balance(ticket_itr->owner,accept_symbol).amount;
            const char* mixedChar2 = reinterpret_cast<const char *>(&mixd2);
            new_seed=sha256((char *)mixedChar2, sizeof(mixedChar2));
            lcount=0;
        }
        ticket_itr++;
    }
    
    auto mixd3=new_seed.data()+now();
    
    const char* mixedChar3 = reinterpret_cast<const char *>(&mixd3);
    new_seed=sha256((char *)mixedChar3, sizeof(mixedChar3));
    

    
    
    
    seed_tbl.modify(seed_itr,get_self(),[&](auto &row){
                row.seed=new_seed;
            });

    
    

    vector <int> numbers;
    bool n[39]={true};
    for(int i=0;i<39;i++){
        n[i]=true;
    }
    long jj;
    int tn;
    for(int i=0;i<5;i++){
            
            jj=(long)(((int)new_seed.extract_as_byte_array()[i*5]+(int)new_seed.extract_as_byte_array()[i*5+1]+(int)new_seed.extract_as_byte_array()[i*5+2])%(39-i));
            tn=0;
            while(jj!=0){
                tn++;
                if(n[tn]){
                    jj--;
                }
                
            }
            print(tn+1);
            print(",");
            n[tn]=false;
            numbers.push_back(tn+1);
        }

   
    
    

    

    int temp[6]={0,0,0,0,0,0};
    int count;
    ticket_itr=ticket_tbl.begin(); 
    auto balance=get_balance(get_self(),accept_symbol).amount;
    eosio::asset quantity;
    auto new_allow_itr=allow_tbl.begin();

    while(ticket_itr!=ticket_tbl.end()){
        new_allow_itr=allow_tbl.find(stoll(ticket_itr->period));
        if(ticket_itr->period==period){
            quantity=ticket_itr->betfunds;
            count=open(ticket_itr->numbers,numbers,quantity);
            if(count>1&&ticket_itr->period==period){
                    balance-=quantity.amount;
                }
            temp[count]++;

            }
        
        else if(new_allow_itr->status==0){
            
            quantity=ticket_itr->betfunds;
            count=open(ticket_itr->numbers,new_allow_itr->numbers,quantity);
            if(count>1&&ticket_itr->period==period){
                    balance-=quantity.amount;
                }
            
            
            }
        ticket_itr++;
        }
        check(balance>0,"error:Bankruptcy");
    vector<int> people;
    for(int i=0;i<6;i++){
        people.push_back(temp[i]);
        }
    allow_tbl.modify(allow_itr,get_self(),[&](auto &row){
                row.numbers.assign(numbers.begin(), numbers.end());
                row.status=0;
                row.opentime=now();
                row.people_win_prize.assign(people.begin(), people.end());
                row.currenpeoplet_balance=asset(balance,accept_symbol);
        });
      
    allow_itr++;
    allow_tbl.modify(allow_itr,get_self(),[&](auto &row){
                row.status=1;
        });
}


