/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "eosio.token.hpp"
#include <eosio/chain/fioio/fioerror.hpp>
#include <fio.common/fio.common.hpp>
#include <fio.common/json.hpp>
using namespace fioio;
namespace eosio {

void token::create( account_name issuer,
                    asset        maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.name() );
    auto existing = statstable.find( sym.name() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void token::issue( account_name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_name = sym.name();
    stats statstable( _self, sym_name );
    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, 0, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
       SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
    }
}

void token::transfer( account_name from,
                      account_name to,
                      asset        quantity,
                      string       memo )
{
    eosio_assert( from != to, "Invalid FIO Public Address" );
    require_auth( from );
    eosio_assert( is_account( to ), "Invalid FIO Public Address");
    auto sym = quantity.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );


    sub_balance( from, quantity );
    add_balance( to, quantity, from );
}

void token::transferfio( name      tofiopubadd,
                         string       amount,
                         name         actor )
{
    fio_400_assert(actor != tofiopubadd, "tofiopubadd", tofiopubadd.to_string(), "Invalid FIO Public Address", ErrorPubAddressEmpty);
    require_auth( actor );
    fio_400_assert(is_account( tofiopubadd ),"tofiopubadd", tofiopubadd.to_string(), "Invalid FIO Public Address", ErrorPubAddressExist);
    string whole;
    string precision;
    size_t pos = amount.find('.');
    whole = amount.substr(0,pos);
    precision = amount.substr(pos+1,amount.size());
    asset qty;
    qty.amount = ((int64_t)atoi(whole.c_str())*10000) + ((int64_t)atoi(precision.c_str()));
    qty.symbol = ::eosio::string_to_symbol(4,"FIO");

    auto sym = qty.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_recipient( actor );
    require_recipient( tofiopubadd );

    fio_400_assert(qty.is_valid(),"amount", amount.c_str(), "Invalid quantity", ErrorLowFunds);
    eosio_assert( qty.amount > 0, "must transfer positive quantity" );
    eosio_assert( qty.symbol == st.supply.symbol, "symbol precision mismatch" );
  //  eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    sub_balance( actor, qty );
    add_balance( tofiopubadd, qty, actor );


    nlohmann::json json = {{"status",     "OK"}};
    send_response(json.dump().c_str());
}


void token::sub_balance( account_name owner, asset value ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.symbol.name(), "Insufficient balance" );
   fio_400_assert(from.balance.amount >= value.amount,"amount", "", "Insufficient balance", ErrorLowFunds);


   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }
}

void token::add_balance( account_name owner, asset value, account_name ram_payer )
{
   accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.symbol.name() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, 0, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

} /// namespace eosio

EOSIO_ABI( eosio::token, (create)(issue)(transfer)(transferfio) )
