/** FIO Request OBT header file
 *  Description: The FIO request other blockchain transaction (or obt) contract contains the
 *  tables that hold the requests for funds that have been made along with the status of any
 *  response to each request for funds.
 *  @author Adam Androulidakis, Casey Gardiner, Ciju John, Ed Rotthoff
 *  @file fio.request.obt.hpp
 *  @copyright Dapix
 */


#pragma once

#include <eosio/eosio.hpp>
#include <string>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>

using std::string;

namespace fioio {

    using namespace eosio;

    // The status of a transaction progresses from requested, to either rejected or sent to blockchain (if funds
    // are sent in response to the request.
    enum class trxstatus {
        requested = 0,
        rejected = 1,
        sent_to_blockchain = 2
    };

    // The request context table holds the requests for funds that have been requested, it provides
    // searching by id, payer and payee.
    // @abi table fioreqctxts i64
    struct [[eosio::action]] fioreqctxt {
        uint64_t fio_request_id;
        uint128_t payer_fio_address;
        uint128_t payee_fio_address;
        string payer_fio_address_hex_str;
        string payee_fio_address_hex_str;
        string content;  //this content is a encrypted blob containing the details of the request.
        uint64_t time_stamp;
        string payer_fio_addr;
        string payee_fio_addr;

        uint64_t primary_key() const { return fio_request_id; }
        uint128_t by_receiver() const { return payer_fio_address; }
        uint128_t by_originator() const { return payee_fio_address; }

        EOSLIB_SERIALIZE(fioreqctxt,
        (fio_request_id)(payer_fio_address)(payee_fio_address)(payer_fio_address_hex_str)(payee_fio_address_hex_str)(
                content)(time_stamp)(payer_fio_addr)(payee_fio_addr)
        )
    };


    typedef multi_index<"fioreqctxts"_n, fioreqctxt,
            indexed_by<"byreceiver"_n, const_mem_fun < fioreqctxt, uint128_t, &fioreqctxt::by_receiver>>,
    indexed_by<"byoriginator"_n, const_mem_fun<fioreqctxt, uint128_t, &fioreqctxt::by_originator>
    >>
    fiorequest_contexts_table;

    // The FIO request status table references FIO requests that have had funds sent, or have been rejected.
    // the table provides a means to find the status of a request.
    // @abi table fioreqstss i64
    struct [[eosio::action]] fioreqsts {
        uint64_t id;
        uint64_t fio_request_id;
        uint64_t status;
        string metadata;   //this contains a json string with meta data regarding the response to a request.
        uint64_t time_stamp;

        uint64_t primary_key() const { return id; }
        uint64_t by_fioreqid() const { return fio_request_id; }

        EOSLIB_SERIALIZE(fioreqsts, (id)(fio_request_id)(status)(metadata)(time_stamp)
        )
    };

    typedef multi_index<"fioreqstss"_n, fioreqsts,
            indexed_by<"byfioreqid"_n, const_mem_fun < fioreqsts, uint64_t, &fioreqsts::by_fioreqid> >
    >
    fiorequest_status_table;
}
