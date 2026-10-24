
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>

#include <grpcpp/grpcpp.h>
#include "file_processor.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;

using namespace file_processor;
namespace fs = std::filesystem;

class FileProcessorServiceImpl final
    : public FileProcessorService::Service {

private:

    void LogOperation(
        const std::string& type,
        const std::string& service,
        const std::string& file,
        const std::string& msg
    ) {

        std::ofstream log("server.log", std::ios::app);

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        log << std::put_time(
            std::localtime(&time),
            "%Y-%m-%d %H:%M:%S"
        );

        log << " | " << type;
        log << " | " << service;
        log << " | " << file;
        log << " | " << msg << std::endl;
    }

    bool ReceiveFile(
        ServerReaderWriter<FileResponse, FileRequest>* stream,
        std::string& filepath,
        std::string& original_name,
        FileRequest& last_req
    ) {

        FileRequest req;
        std::ofstream outfile;
        bool first = true;

        while (stream->Read(&req)) {

            if (first) {

                original_name = req.file_name();

                filepath = "/tmp/input_" +
                           std::to_string(
                               std::chrono::steady_clock::now()
                               .time_since_epoch().count()
                           );

                outfile.open(filepath, std::ios::binary);

                last_req = req;

                first = false;
            }

            if (req.has_file_content()) {

                outfile.write(
                    req.file_content().content().data(),
                    req.file_content().content().size()
                );
            }
        }

        if (outfile.is_open())
            outfile.close();

        return !first;
    }

    Status SendFile(
        ServerReaderWriter<FileResponse, FileRequest>* stream,
        const std::string& out_filepath,
        const std::string& out_filename
    ) {

        std::ifstream infile(out_filepath, std::ios::binary);

        if (!infile) {

            return Status(
                grpc::StatusCode::INTERNAL,
                "Falha ao ler arquivo de saida"
            );
        }

        char buffer[1024];

        while (
            infile.read(buffer, sizeof(buffer)) ||
            infile.gcount() > 0
        ) {

            FileResponse resp;

            resp.set_success(true);
            resp.set_file_name(out_filename);
            resp.set_status_message("Sucesso");

            resp.mutable_file_content()->set_content(
                buffer,
                infile.gcount()
            );

            if (!stream->Write(resp)) {
                return Status(
                    grpc::StatusCode::CANCELLED,
                    "Cliente desconectado"
                );
            }
        }

        return Status::OK;
    }

public:

    Status CompressPDF(
        ServerContext* context,
        ServerReaderWriter<FileResponse, FileRequest>* stream
    ) override {

        std::string input_path;
        std::string original_name;
        FileRequest last_req;

        if (!ReceiveFile(
            stream,
            input_path,
            original_name,
            last_req
        )) {

            LogOperation(
                "ERRO",
                "CompressPDF",
                original_name,
                "Arquivo vazio"
            );

            return Status(
                grpc::StatusCode::INVALID_ARGUMENT,
                "Arquivo vazio"
            );
        }

        std::string output_path = input_path + "_compressed.pdf";

        std::string command =
            "gs -sDEVICE=pdfwrite "
            "-dCompatibilityLevel=1.4 "
            "-dPDFSETTINGS=/ebook "
            "-dNOPAUSE -dQUIET -dBATCH "
            "-sOutputFile=\"" + output_path +
            "\" \"" + input_path + "\"";

        int result = std::system(command.c_str());

        if (result != 0) {

            LogOperation(
                "ERRO",
                "CompressPDF",
                original_name,
                "Falha na compressao"
            );

            std::remove(input_path.c_str());

            return Status(
                grpc::StatusCode::INTERNAL,
                "Erro ao comprimir PDF"
            );
        }

        LogOperation(
            "SUCESSO",
            "CompressPDF",
            original_name,
            "PDF comprimido"
        );

        Status status = SendFile(
            stream,
            output_path,
            "comprimido.pdf"
        );

        std::remove(input_path.c_str());
        std::remove(output_path.c_str());

        return status;
    }

    Status ConvertToTXT(
        ServerContext*,
        ServerReaderWriter<FileResponse, FileRequest>*
    ) override {

        return Status(
            grpc::StatusCode::UNIMPLEMENTED,
            "Servico nao implementado"
        );
    }

    Status ConvertImageFormat(
        ServerContext*,
        ServerReaderWriter<FileResponse, FileRequest>*
    ) override {

        return Status(
            grpc::StatusCode::UNIMPLEMENTED,
            "Servico nao implementado"
        );
    }

    Status ResizeImage(
        ServerContext*,
        ServerReaderWriter<FileResponse, FileRequest>*
    ) override {

        return Status(
            grpc::StatusCode::UNIMPLEMENTED,
            "Servico nao implementado"
        );
    }
};

void RunServer() {

    std::string server_address("0.0.0.0:50051");

    FileProcessorServiceImpl service;

    ServerBuilder builder;

    builder.AddListeningPort(
        server_address,
        grpc::InsecureServerCredentials()
    );

    builder.RegisterService(&service);

    std::unique_ptr<Server> server(
        builder.BuildAndStart()
    );

    std::cout
        << "Servidor gRPC operacional em "
        << server_address
        << std::endl;

    server->Wait();
}

int main() {

    RunServer();

    return 0;
}
