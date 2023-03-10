/*
MIT License

Copyright(c) 2022 Futurewei Cloud

    Permission is hereby granted,
    free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

    The above copyright notice and this permission notice shall be included in all copies
    or
    substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS",
    WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    DAMAGES OR OTHER
    LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once

#include <k2/config/Config.h>
#include <k2/module/k23si/client/k23si_client.h>
#include <boost/intrusive/list.hpp>
#include <k2/common/MapWithExpiry.h>

namespace k2 {


class HTTPProxy {
public:  // application lifespan
    HTTPProxy();
    seastar::future<> gracefulStop();
    seastar::future<> start();

private:
    static void serializeRecordFromJSON(k2::dto::SKVRecord& record, nlohmann::json&& jsonRecord);
    static nlohmann::json serializeJSONFromRecord(k2::dto::SKVRecord& record);

    seastar::future<nlohmann::json> _handleBegin(nlohmann::json&& request);
    seastar::future<nlohmann::json> _handleEnd(nlohmann::json&& request);
    seastar::future<nlohmann::json> _handleRead(nlohmann::json&& request);
    seastar::future<nlohmann::json> _handleWrite(nlohmann::json&& request);
    seastar::future<nlohmann::json> _handleGetKeyString(nlohmann::json&& request);
    seastar::future<nlohmann::json> _handleCreateQuery(nlohmann::json&& request);
    seastar::future<nlohmann::json> _handleQuery(nlohmann::json&& request);

    seastar::future<std::tuple<k2::Status, dto::CreateSchemaResponse>> _handleCreateSchema(
        dto::CreateSchemaRequest&& request);
    seastar::future<std::tuple<k2::Status, dto::Schema>> _handleGetSchema(
        dto::GetSchemaRequest&& request);
    seastar::future<std::tuple<k2::Status, dto::CollectionCreateResponse>> _handleCreateCollection(
        dto::CollectionCreateRequest&& request);

    void _registerAPI();
    void _registerMetrics();

    sm::metric_groups _metric_groups;
    uint64_t _deserializationErrors = 0;
    uint64_t _timedoutTxns = 0;
    uint64_t _numQueries = 0;

    bool _stopped = true;
    k2::K23SIClient _client;
    uint64_t _txnID = 0;
    uint64_t _queryID = 0;

    struct TxnInfo {
        k2::K2TxnHandle txn;
        // Store in progress queries
        std::unordered_map<uint64_t, Query> queries;
        // clanup function is required for MapWithExpiry
        auto cleanup() {
            return txn.end(false);
        }
    };

    MapWithExpiry<uint64_t, TxnInfo> _txns;
    // Txn idle timeout
    ConfigDuration httpproxy_txn_timeout{"httpproxy_txn_timeout", 60s};
};  // class HTTPProxy

} // namespace k2
