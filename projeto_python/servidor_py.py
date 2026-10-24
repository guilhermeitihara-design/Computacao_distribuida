import grpc
from concurrent import futures

import calculator_pb2
import calculator_pb2_grpc


class ServicoCalculadora(
    calculator_pb2_grpc.CalculadoraServicer
):
    def Somar(self, request, context):
        print(
            f"Servidor Python: Recebido "
            f"{request.a} + {request.b}"
        )

        resultado = request.a + request.b

        return calculator_pb2.SomaResponse(
            resultado=resultado
        )


def serve():
    server = grpc.server(
        futures.ThreadPoolExecutor(max_workers=10)
    )

    calculator_pb2_grpc.add_CalculadoraServicer_to_server(
        ServicoCalculadora(),
        server
    )

    server.add_insecure_port("[::]:50051")

    server.start()

    print(
        "Servidor Python escutando na porta 50051"
    )

    server.wait_for_termination()


if __name__ == "__main__":
    serve()
