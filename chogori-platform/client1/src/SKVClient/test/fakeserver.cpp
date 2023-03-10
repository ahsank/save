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

// Code to test http serialization/deserialization from client

#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <skv/client/SKVClient.h>

using namespace skv::http;

const char *response_type = "application/x-msgpack";

template <typename T>
    Binary _serialize(T&& obj) {
    MPackWriter writer;
    Response<T> resp(Statuses::S200_OK, std::forward<T>(obj));
    writer.write(resp);
    Binary buf;
    if (!writer.flush(buf)) {
        throw std::runtime_error("Serialization failed");
    }
    return buf;
}

void setError(httplib::Response &resp, const Status& status) {
    K2LOG_E(log::dto, "Sent  error {}", status);
    resp.status = status.code;
    resp.set_content(status.message, "text/plain");
}

template <typename T>
void _sendResponse(httplib::Response &resp, T& obj) {
    auto&& buf = _serialize(obj);
    resp.status = 200;
    resp.set_content(buf.data(), buf.size(), response_type);
 }

template <typename T>
T _deserialize(const std::string& body) {
    String copy(body);
    Binary buf(std::move(copy));
    MPackReader reader(buf);
    T obj;
    if (!reader.read(obj)) {
        throw dto::DeserializationError();
    }
    return obj;
}

struct ServerError : public std::runtime_error {
    Status status;
    ServerError(const Status& s) : std::runtime_error(s.message), status(s) {}
};

template<typename ReqT, typename RespT>
void process(httplib::Server& svr, const char *path, std::function<RespT(ReqT&&)> fn) {
    svr.Post(path, [fn](const httplib::Request &req, httplib::Response &resp) {
        try {
            ReqT reqObj = _deserialize<ReqT>(req.body);
            K2LOG_I(log::dto, "Got {} request {}", req.path, reqObj);
            RespT respObj = fn(std::move(reqObj));
            _sendResponse(resp, respObj);
            K2LOG_I(log::dto, "Sent  {}", respObj);
        }  catch(ServerError& e) {
            setError(resp, e.status);
        } catch(std::runtime_error& e) {
            setError(resp, Statuses::S500_Internal_Server_Error);
        }
    });
}

namespace po = boost::program_options;
po::options_description desc("Allowed options");
po::variables_map vm;

std::shared_ptr<dto::Schema> getSchema() {
    static std::shared_ptr<dto::Schema> schemaPtr;
    if (!schemaPtr) {
        dto::Schema schema;
        schema.name = "test_schema";
        schema.version = 1;
        schema.fields = std::vector<dto::SchemaField> {
            {dto::FieldType::STRING, "LastName", false, false},
            {dto::FieldType::STRING, "FirstName", false, false},
            {dto::FieldType::INT32T, "Balance", false, false}
        };
        schema.setPartitionKeyFieldsByName(std::vector<String>{"LastName"});
        schema.setRangeKeyFieldsByName(std::vector<String>{"FirstName"});
        schemaPtr = std::make_shared<dto::Schema>(schema);
    }
    return schemaPtr;
}

using Storage = dto::SKVRecord::Storage;

std::string_view get_key(const Storage& storage) {
    return std::string_view(storage.fieldData.data(), storage.fieldData.size());
}

template <>
struct std::less<Storage>
{
    bool operator()(const Storage &a, const Storage &b) const {return get_key(a) < get_key(b);}
};

int main(int argc, char **argv) {
    desc.add_options()
    ("help", "produce help message")
    ("port", po::value<int>()->default_value(30000), "server port");

    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    // HTTP
    httplib::Server svr;
    uint64_t txnId = 0;
    std::map<Storage, Storage, std::less<Storage>> data;
    std::set<String> collections = {"test_collecction"};
    std::map<std::tuple<String, int>, std::shared_ptr<dto::Schema>> schemas = {
        {{"test_schema", 1}, getSchema()}
    };

    process<dto::K23SIBeginTxnRequest, dto::K23SIBeginTxnResponse>(svr, "/api/v1/beginTxn", [&] (auto&&) {
        return  dto::K23SIBeginTxnResponse{dto::Timestamp(100000, 1, txnId++)};
    });

    process<dto::K23SITxnEndRequest, dto::K23SITxnEndResponse>(svr, "/api/v1/endTxn", [] (auto&&) {
        return  dto::K23SITxnEndResponse{};
    });

    process<dto::K23SIWriteRequest, dto::K23SIWriteResponse>(svr, "/api/v1/write", [&] (auto&& req) {
        auto schema = schemas.find({req.schemaName, req.schemaVersion});
        if (collections.find(req.collectionName) == collections.end()|| schema == schemas.end()) {
            throw ServerError(Statuses::S404_Not_Found);
        }
        dto::SKVRecord rec(req.collectionName, schema->second, std::move(req.value), true);
        data[rec.getSKVKeyRecord().storage] = std::move(rec.storage);
        return  dto::K23SIWriteResponse{};
    });

    process<dto::K23SIReadRequest, dto::K23SIReadResponse>(svr, "/api/v1/read", [&] (auto&& req) {
        auto schema = schemas.find({req.schemaName, req.schemaVersion});
        if (collections.find(req.collectionName) == collections.end()|| schema == schemas.end()) {
            throw ServerError(Statuses::S404_Not_Found);
        }
        if (auto it = data.find(req.key); it != data.end()) {
            return dto::K23SIReadResponse{it->second.copy()};
        }
        throw  ServerError(Statuses::S404_Not_Found);
    });

    svr.listen("0.0.0.0", vm["port"].as<int>());
}

