#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <queue>
#include <array>
#include <algorithm>
#include <iostream>
#include <string>

// Dodajte ove deklaracije na početak fajla
std::string putTextString(int rank);
std::string suitToString(int suit) {
    const std::vector<std::string> names = {"Hearts", "Diamonds", "Clubs", "Spades"};
    return (suit >= 0 && suit < 4) ? names[suit] : "Unknown";
}

int rankMatcher(const std::vector<unsigned char>& rnk_img, int rnk_width, int rnk_height);
int matchSuit(const std::vector<unsigned char>& suitImg, int width, int height);


std::vector<unsigned char> load_image_grayscale(const std::string& filepath, int& width, int& height) {
    // Učitavanje slike u RGB formatu
    int channels;
    unsigned char* image = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

    if (image == nullptr) {
        std::cerr << "Ne mogu da učitam sliku: " << filepath << std::endl;
        exit(1);
    }

    std::vector<unsigned char> grayscale_data;
    grayscale_data.reserve(width * height);

    // Pretvaranje slike u grayscale (koristeći jednostavan weighted sum za RGB)
    for (int i = 0; i < width * height; ++i) {
        int r = image[i * channels + 0];
        int g = image[i * channels + 1];
        int b = image[i * channels + 2];
        unsigned char gray = static_cast<unsigned char>(0.3 * r + 0.59 * g + 0.11 * b); // Standardna formula za grayscale
        grayscale_data.push_back(gray);
    }

    // Oslobađanje memorije za originalnu sliku
    stbi_image_free(image);
    return grayscale_data;
}

struct Point2f {
    float x, y;
};

float distance(Point2f a, Point2f b) {
    return std::sqrt((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
} //pitagora

std::array<Point2f, 4> find_corners(const std::vector<Point2f>& points) {
    Point2f topLeft = points[0], topRight = points[0], bottomRight = points[0], bottomLeft = points[0];
    float minSum = 1e9, maxSum = -1e9, minDiff = 1e9, maxDiff = -1e9;

    for (const auto& p : points) {
        float sum = p.x + p.y;
        float diff = p.x - p.y;

        if (sum < minSum) { minSum = sum; topLeft = p; }
        if (sum > maxSum) { maxSum = sum; bottomRight = p; }
        if (diff < minDiff) { minDiff = diff; topRight = p; }
        if (diff > maxDiff) { maxDiff = diff; bottomLeft = p; }
    }

    float widthA = distance(topLeft, topRight);
    float widthB = distance(bottomLeft, bottomRight);
    float heightA = distance(topLeft, bottomLeft);
    float heightB = distance(topRight, bottomRight);

    float avgWidth = (widthA + widthB) / 2.0f;
    float avgHeight = (heightA + heightB) / 2.0f;

    if (avgWidth > avgHeight) {
        return { bottomLeft, topLeft, topRight, bottomRight };
    }

    return { topLeft, topRight, bottomRight, bottomLeft };
}

std::vector<unsigned char> binarize_image(const std::vector<unsigned char>& gray, int width, int height, int threshold) {
    std::vector<unsigned char> binary(width * height);
    for (int i = 0; i < width * height; ++i)
        binary[i] = (gray[i] > threshold) ? 255 : 0;
    return binary;
}

std::vector<Point2f> find_largest_component(const std::vector<unsigned char>& binary, int width, int height) {
    std::vector<bool> visited(width * height, false);
    std::vector<Point2f> largest;
    const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            if (binary[idx] == 255 && !visited[idx]) {
                std::vector<Point2f> region;
                std::queue<Point2f> q;
                q.push({(float)x, (float)y});
                visited[idx] = true;

                while (!q.empty()) {
                    Point2f p = q.front(); q.pop();
                    region.push_back(p);
                    for (int d = 0; d < 4; ++d) {
                        int nx = p.x + dx[d];
                        int ny = p.y + dy[d];
                        if (nx >= 0 && ny >= 0 && nx < width && ny < height) {
                            int nidx = ny * width + nx;
                            if (binary[nidx] == 255 && !visited[nidx]) {
                                visited[nidx] = true;
                                q.push({(float)nx, (float)ny});
                            }
                        }
                    }
                }

                if (region.size() > largest.size())
                    largest = region;
            }
        }
    return largest;
}





void save_image(const std::string& filename, const std::vector<unsigned char>& image, int width, int height) {
    stbi_write_png(filename.c_str(), width, height, 3, image.data(), width * 3);
}

std::vector<unsigned char> warp_image(const unsigned char* input, int srcW, int srcH, std::array<Point2f, 4> corners, int dstW, int dstH) {
    std::vector<unsigned char> output(dstW * dstH * 3);

    for (int y = 0; y < dstH; ++y) {
        float v = y / (float)(dstH - 1);
        for (int x = 0; x < dstW; ++x) {
            float u = x / (float)(dstW - 1);

            Point2f top = {
                (1 - u) * corners[0].x + u * corners[1].x,
                (1 - u) * corners[0].y + u * corners[1].y
            };
            Point2f bottom = {
                (1 - u) * corners[3].x + u * corners[2].x,
                (1 - u) * corners[3].y + u * corners[2].y
            };
            Point2f p = {
                (1 - v) * top.x + v * bottom.x,
                (1 - v) * top.y + v * bottom.y
            };

            int px = std::clamp((int)p.x, 0, srcW - 1);
            int py = std::clamp((int)p.y, 0, srcH - 1);
            for (int c = 0; c < 3; ++c)
                output[(y * dstW + x) * 3 + c] = input[(py * srcW + px) * 3 + c];
        }
    }

    // Flip horizontally to ensure number and symbol are in top-left
    std::vector<unsigned char> flipped(dstW * dstH * 3);
    for (int y = 0; y < dstH; ++y) {
        for (int x = 0; x < dstW; ++x) {
            for (int c = 0; c < 3; ++c) {
                flipped[(y * dstW + x) * 3 + c] = output[(y * dstW + (dstW - 1 - x)) * 3 + c];
            }
        }
    }
    return flipped;
}

void save_top_left_corner(const std::vector<unsigned char>& image, int width, int height, int cornerW, int cornerH, const std::string& filename) {
    std::vector<unsigned char> cropped(cornerW * cornerH * 3);
    for (int y = 0; y < cornerH; ++y) {
        for (int x = 0; x < cornerW; ++x) {
            for (int c = 0; c < 3; ++c) {
                cropped[(y * cornerW + x) * 3 + c] = image[(y * width + x) * 3 + c];
            }
        }
    }
    stbi_write_png(filename.c_str(), cornerW, cornerH, 3, cropped.data(), cornerW * 3);
}

int get_image_height(const std::string& filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 3);
    if (!data) {
        std::cerr << "Greska pri ucitavanju slike: " << filename << std::endl;
        return -1;
    }
    stbi_image_free(data);
    return height;
}


void split_symbol_image(const std::string& filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 3);
    if (!data) {
        std::cerr << "Greska pri ucitavanju slike: " << filename << std::endl;
        return;
    }

   // int mid = height / 2;
    int mid = static_cast<int>(height * 0.60); 
    // Gornja polovina - "broj.png"
    std::vector<unsigned char> topHalf(width * mid * 3);
    for (int y = 0; y < mid; ++y)
        for (int x = 0; x < width; ++x)
            for (int c = 0; c < 3; ++c)
                topHalf[(y * width + x) * 3 + c] = data[(y * width + x) * 3 + c];

    stbi_write_png("broj.png", width, mid, 3, topHalf.data(), width * 3);

    // Donja polovina - "znak.png"
    int bottomH = height - mid;
    std::vector<unsigned char> bottomHalf(width * bottomH * 3);
    for (int y = 0; y < bottomH; ++y)
        for (int x = 0; x < width; ++x)
            for (int c = 0; c < 3; ++c)
                bottomHalf[(y * width + x) * 3 + c] = data[((y + mid) * width + x) * 3 + c];

    stbi_write_png("znak.png", width, bottomH, 3, bottomHalf.data(), width * 3);

    stbi_image_free(data);
}

int count_white_pixels(const std::vector<unsigned char>& img, int width, int height) {
    int count = 0;
    for (int i = 0; i < width*height; i++) {
        if (img[i] == 255) count++;
    }
    return count;
}

std::vector<unsigned char> abs_diff(const std::vector<unsigned char>& img1, 
                                     const std::vector<unsigned char>& img2, 
                                     int width, int height) {
    std::vector<unsigned char> diff(img1.size());
    for (int i = 0; i < img1.size(); ++i) {
        diff[i] = std::abs(img1[i] - img2[i]);
    }
    return diff;
}



// Implementacija putTextString (premještena prije rankMatcher)
std::string putTextString(int rank) {
    static const std::vector<std::string> rank_names = {
        "Unknown",    // 0
        "Two",        // 1
        "Three",      // 2
        "Four",       // 3
        "Five",       // 4
        "Six",        // 5
        "Seven",      // 6
        "Eight",      // 7
        "Nine",       // 8
        "Ten",        // 9
        "Jack",       // 10
        "Queen",      // 11
        "King",       // 12
        "Ace"         // 13
    };

    if (rank >= 0 && rank < static_cast<int>(rank_names.size())) {
        return rank_names[rank];
    }
    return "Unknown";
}

std::vector<unsigned char> binarize(const unsigned char* data, int width, int height, int channels, int threshold = 120) {
    std::vector<unsigned char> binary(width * height);
    for (int i = 0; i < width * height; i++) {
        unsigned char gray = 0;
if (channels == 1) {
    gray = data[i];
} else {
    gray = data[i * channels] * 0.3;
    if (channels > 1) gray += data[i * channels + 1] * 0.59;
    if (channels > 2) gray += data[i * channels + 2] * 0.11;
}

        binary[i] = (gray > threshold) ? 255 : 0;
    }
    return binary;
}
/*
// Implementacija rankMatcher
int rankMatcher(const std::vector<unsigned char>& rankImg, int width, int height) {
    const std::vector<std::pair<std::string, int>> templates = {
        {"Card_Imgs/Ranks/2.jpg", 1}, {"Card_Imgs/Ranks/3.jpg", 2},
        {"Card_Imgs/Ranks/4.jpg", 3}, {"Card_Imgs/Ranks/5.jpg", 4},
        {"Card_Imgs/Ranks/6.jpg", 5}, {"Card_Imgs/Ranks/7.jpg", 6},
        {"Card_Imgs/Ranks/8.jpg", 7}, {"Card_Imgs/Ranks/9.jpg", 8},
        {"Card_Imgs/Ranks/0.jpg", 9}, {"Card_Imgs/Ranks/jack.jpg", 10},
        {"Card_Imgs/Ranks/queen.jpg", 11}, {"Card_Imgs/Ranks/king.jpg", 12},
        {"Card_Imgs/Ranks/ace.jpg", 13}
    };

    int bestMatch = -1;
    int minDiff = INT_MAX;

    std::cout << "[INFO] Inverting input image...\n";
    std::vector<unsigned char> inverted(rankImg.size());
    for (size_t i = 0; i < rankImg.size(); ++i) {
        inverted[i] = 255 - rankImg[i];
    }

    stbi_write_png("_debug_input_rank.png", width, height, 1, inverted.data(), width);

    for (const auto& [file, rank] : templates) {
        std::cout << "[INFO] Loading template: " << file << std::endl;
        int tplW, tplH, tplC;
        unsigned char* tplData = stbi_load(file.c_str(), &tplW, &tplH, &tplC, 0);
        if (!tplData) {
            std::cerr << "[ERROR] Failed to load template " << file << std::endl;
            continue;
        }

        std::vector<unsigned char> tplBinary = binarize(tplData, tplW, tplH, tplC);
        std::string outName = "_debug_tpl_rank_" + std::to_string(rank) + ".png";
        stbi_write_png(outName.c_str(), tplW, tplH, 1, tplBinary.data(), tplW);

        int diff = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int tplX = x * tplW / width;
                int tplY = y * tplH / height;
                if (inverted[y * width + x] != tplBinary[tplY * tplW + tplX]) {
                    diff++;
                }
            }
        }
        
        

        std::cout << " -> Rank " << rank << " has diff: " << diff << std::endl;

        if (diff < minDiff) {
            minDiff = diff;
            bestMatch = rank;
        }

        stbi_image_free(tplData);
    }

    std::cout << "[RESULT] Best match rank: " << bestMatch << std::endl;
    return bestMatch;
}


// Funkcija za invertovanje boja slike
void invert_image(std::vector<unsigned char>& img, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        img[i] = 255 - img[i]; // Obrati boje (crno postaje belo, i obrnuto)
    }
}

*/

/*// Funkcija za prepoznavanje "suit-a" sa upotrebom šablona
int matchSuit(const std::vector<unsigned char>& suitImg, int width, int height) {
    const std::vector<std::pair<std::string, int>> templates = {
        {"Card_Imgs/Suits/hearts.jpg", 0},  // Hearts
        {"Card_Imgs/Suits/diamonds.jpg", 1},  // Diamonds
        {"Card_Imgs/Suits/clubs.jpg", 2},  // Clubs
        {"Card_Imgs/Suits/spades.jpg", 3}   // Spades
    };
     int bestMatch = -1;
    int minDiff = INT_MAX;

    std::cout << "[INFO] Inverting input image...\n";
    std::vector<unsigned char> inverted1(suitImg.size());
    for (size_t i = 0; i < suitImg.size(); ++i) {
        inverted1[i] = 255 - suitImg[i];
    }

    stbi_write_png("_debug_input_suit.png", width, height, 1, inverted1.data(), width);

    for (const auto& [file, suit] : templates) {
        std::cout << "[INFO] Loading template: " << file << std::endl;
        int tplW, tplH, tplC;
        unsigned char* tplData = stbi_load(file.c_str(), &tplW, &tplH, &tplC, 0);
        if (!tplData) {
            std::cerr << "[ERROR] Failed to load template " << file << std::endl;
            continue;
        }

        std::vector<unsigned char> tplBinary = binarize(tplData, tplW, tplH, tplC);
        std::string outName = "_debug_tpl_suit_" + std::to_string(suit) + ".png";
        stbi_write_png(outName.c_str(), tplW, tplH, 1, tplBinary.data(), tplW);

        int diff = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int tplX = x * tplW / width;
                int tplY = y * tplH / height;
                if (inverted1[y * width + x] != tplBinary[tplY * tplW + tplX]) {
                    diff++;
                }
            }
        }
        
        

        std::cout << " -> Suit " << suit << " has diff: " << diff << std::endl;

        if (diff < minDiff) {
            minDiff = diff;
            bestMatch = suit;
        }

        stbi_image_free(tplData);
    }

    std::cout << "[RESULT] Best match suit: " << bestMatch << std::endl;
    return bestMatch;
}*/

int rankMatcher(const std::vector<unsigned char>& rankImg, int width, int height) {
    const std::vector<std::pair<std::string, int>> templates = {
        {"Card_Imgs/Ranks/2.jpg", 1}, {"Card_Imgs/Ranks/3.jpg", 2},
        {"Card_Imgs/Ranks/4.jpg", 3}, {"Card_Imgs/Ranks/5.jpg", 4},
        {"Card_Imgs/Ranks/6.jpg", 5}, {"Card_Imgs/Ranks/7.jpg", 6},
        {"Card_Imgs/Ranks/8.jpg", 7}, {"Card_Imgs/Ranks/9.jpg", 8},
        {"Card_Imgs/Ranks/0.jpg", 9}, {"Card_Imgs/Ranks/jack.jpg", 10},
        {"Card_Imgs/Ranks/queen.jpg", 11}, {"Card_Imgs/Ranks/king.jpg", 12},
        {"Card_Imgs/Ranks/ace.jpg", 13}
    };

    int bestMatch = -1;
    int minDiff = INT_MAX;

    // Invertujemo sliku za obradu
    std::vector<unsigned char> inverted(rankImg.size());
    for (size_t i = 0; i < rankImg.size(); ++i) {
        inverted[i] = 255 - rankImg[i];
    }

    // Pronađi najveću komponentu
    std::vector<Point2f> largest = find_largest_component(inverted, width, height);
    if (largest.empty()) {
        std::cerr << "Nema kontura za obradu!" << std::endl;
        return -1;
    }

    // Pronađi granice konture
    float minX = width, maxX = 0, minY = height, maxY = 0;
    for (const auto& p : largest) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }

    // Dodaj padding i osiguraj granice
    const int padding = 2;
    minX = std::max(0.0f, minX - padding);
    minY = std::max(0.0f, minY - padding);
    maxX = std::min((float)width - 1, maxX + padding);
    maxY = std::min((float)height - 1, maxY + padding);

    // Izreži regiju
    int cropW = maxX - minX;
    int cropH = maxY - minY;
    std::vector<unsigned char> cropped(cropW * cropH);
    for (int y = 0; y < cropH; y++) {
        for (int x = 0; x < cropW; x++) {
            int srcX = x + minX;
            int srcY = y + minY;
            cropped[y * cropW + x] = inverted[srcY * width + srcX];
        }
    }

    // Sačuvaj debug sliku
    stbi_write_png("_debug_cropped_rank.png", cropW, cropH, 1, cropped.data(), cropW);

    for (const auto& [file, rank] : templates) {
        // Učitaj template
        int tplW, tplH, tplC;
        unsigned char* tplData = stbi_load(file.c_str(), &tplW, &tplH, &tplC, 0);
        if (!tplData) continue;

        // Resizeuj cropped sliku na dimenzije template-a
        std::vector<unsigned char> resized(tplW * tplH);
        float xRatio = (float)cropW / tplW;
        float yRatio = (float)cropH / tplH;
        for (int y = 0; y < tplH; y++) {
            for (int x = 0; x < tplW; x++) {
                int srcX = x * xRatio;
                int srcY = y * yRatio;
                resized[y * tplW + x] = cropped[srcY * cropW + srcX];
            }
        }

        // Učitaj i obradi template
        std::vector<unsigned char> tplBinary = binarize(tplData, tplW, tplH, tplC);
        
        // Izračunaj razliku
        int diff = 0;
        for (int i = 0; i < tplW * tplH; i++) {
            if (resized[i] != tplBinary[i]) diff++;
        }

        // Ažuriraj najbolji rezultat
        if (diff < minDiff) {
            minDiff = diff;
            bestMatch = rank;
        }

        stbi_image_free(tplData);
        std::cout << " -> Rank " << rank << " has diff: " << diff << std::endl;
    }

    std::cout << "[RESULT] Best match rank: " << bestMatch << std::endl;
    return bestMatch;
}



std::vector<unsigned char> bilinear_resize(const std::vector<unsigned char>& input, int inputWidth, int inputHeight, int outputWidth, int outputHeight) {
    std::vector<unsigned char> resized(outputWidth * outputHeight);

    for (int y = 0; y < outputHeight; y++) {
        for (int x = 0; x < outputWidth; x++) {
            // Normalizovanje koordinata na originalnu sliku
            float srcX = x * (float)inputWidth / outputWidth;
            float srcY = y * (float)inputHeight / outputHeight;

            int x1 = (int)srcX;
            int y1 = (int)srcY;
            int x2 = std::min(x1 + 1, inputWidth - 1);
            int y2 = std::min(y1 + 1, inputHeight - 1);

            // Bilinearna interpolacija
            float dx = srcX - x1;
            float dy = srcY - y1;
            unsigned char value = (1 - dx) * (1 - dy) * input[y1 * inputWidth + x1] +
                                  dx * (1 - dy) * input[y1 * inputWidth + x2] +
                                  (1 - dx) * dy * input[y2 * inputWidth + x1] +
                                  dx * dy * input[y2 * inputWidth + x2];

            resized[y * outputWidth + x] = value;
        }
    }
    return resized;
}

int matchSuit(const std::vector<unsigned char>& suitImg, int width, int height) {
    const std::vector<std::pair<std::string, int>> templates = {
        {"Card_Imgs/Suits/hearts.jpg", 0},
        {"Card_Imgs/Suits/diamonds.jpg", 1},
        {"Card_Imgs/Suits/clubs.jpg", 2},
        {"Card_Imgs/Suits/spades.jpg", 3}
    };

    int bestMatch = -1;
    int minDiff = INT_MAX;

    // Invertujemo sliku za obradu
    std::vector<unsigned char> inverted(suitImg.size());
    for (size_t i = 0; i < suitImg.size(); ++i) {
        inverted[i] = 255 - suitImg[i];
    }

    // Pronađi najveću komponentu
    std::vector<Point2f> largest = find_largest_component(inverted, width, height);
    if (largest.empty()) {
        std::cerr << "Nema kontura za obradu!" << std::endl;
        return -1;
    }

    // Pronađi granice konture
    float minX = width, maxX = 0, minY = height, maxY = 0;
    for (const auto& p : largest) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }

    // Dodaj padding i osiguraj granice
    const int padding = 2;
    minX = std::max(0.0f, minX - padding);
    minY = std::max(0.0f, minY - padding);
    maxX = std::min((float)width - 1, maxX + padding);
    maxY = std::min((float)height - 1, maxY + padding);

    // Izreži regiju
    int cropW = maxX - minX;
    int cropH = maxY - minY;
    std::vector<unsigned char> cropped(cropW * cropH);
    for (int y = 0; y < cropH; y++) {
        for (int x = 0; x < cropW; x++) {
            int srcX = x + minX;
            int srcY = y + minY;
            cropped[y * cropW + x] = inverted[srcY * width + srcX];
        }
    }

    // Sačuvaj debug sliku pre resize-a
    stbi_write_png("_debug_cropped_suit.png", cropW, cropH, 1, cropped.data(), cropW);

    // Resizeuj cropped sliku na dimenzije template-a
    int tplW = 70, tplH = 100;  // Primer dimenzija za template
    std::vector<unsigned char> resized = bilinear_resize(cropped, cropW, cropH, tplW, tplH);

    // Sačuvaj resized sliku za debagovanje
    stbi_write_png("_debug_resized_suit.png", tplW, tplH, 1, resized.data(), tplW);

    // Iteriraj kroz template slike
    for (const auto& [file, suit] : templates) {
        // Učitaj template
        int tplC;
        unsigned char* tplData = stbi_load(file.c_str(), &tplW, &tplH, &tplC, 0);
        if (!tplData) continue;

        // Učitaj i obradi template
        std::vector<unsigned char> tplBinary = binarize(tplData, tplW, tplH, tplC);

        // Izračunaj razliku
        int diff = 0;
        for (int i = 0; i < tplW * tplH; i++) {
            if (resized[i] != tplBinary[i]) diff++;
        }

        // Ažuriraj najbolji rezultat
        if (diff < minDiff) {
            minDiff = diff;
            bestMatch = suit;
        }

        stbi_image_free(tplData);
        std::cout << " -> Suit " << suit << " has diff: " << diff << std::endl;
    }

    std::cout << "[RESULT] Best match suit: " << bestMatch << std::endl;
    return bestMatch;
}



int main() {
    int width, height, channels;
    
    unsigned char* image = stbi_load("karta.jpeg", &width, &height, &channels, 3);
    if (!image) {
        std::cerr << "Greska pri ucitavanju slike!" << std::endl;
        return 1;
    }

    // 1. Convert to grayscale
    std::vector<unsigned char> gray(width * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 3;
            unsigned char r = image[idx], g = image[idx + 1], b = image[idx + 2];
            gray[y * width + x] = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
        }
    }

    // 2. Binarize image
    auto binary = binarize_image(gray, width, height, 120);
    stbi_write_png("step3_binary.jpg", width, height, 1, binary.data(), width);

    // 3. Find largest component (card)
    auto largest = find_largest_component(binary, width, height);
    if (largest.size() < 100) {
        std::cerr << "Nema dovoljno velika kontura!" << std::endl;
        stbi_image_free(image);
        return 1;
    }

    // 4. Find corners
    auto corners = find_corners(largest);

    // 5. Mark corners on image
    std::vector<unsigned char> cornerImage(image, image + width * height * 3);
    for (auto& c : corners) {
        int xx = (int)c.x, yy = (int)c.y;
        for (int dy = -5; dy <= 5; ++dy) {
            for (int dx = -5; dx <= 5; ++dx) {
                int nx = xx + dx, ny = yy + dy;
                if (nx >= 0 && ny >= 0 && nx < width && ny < height) {
                    int idx = (ny * width + nx) * 3;
                    cornerImage[idx] = 255;
                    cornerImage[idx + 1] = 0;
                    cornerImage[idx + 2] = 0;
                }
            }
        }
    }
    save_image("step4_corners.jpg", cornerImage, width, height);

    // 6. Warp perspective
    auto warped = warp_image(image, width, height, corners, 200, 300);
    save_image("step5_warped.jpg", warped, 200, 300);

    // 7. Extract top-left corner
    save_top_left_corner(warped, 200, 300, 33, 90, "step6_corner_topleft.png");
    
    // 8. Process corner image
    int tlw, tlh, tlch;
    unsigned char* cornerImg = stbi_load("step6_corner_topleft.png", &tlw, &tlh, &tlch, 3);
    if (!cornerImg) {
        std::cerr << "Greska pri ucitavanju step6_corner_topleft.png!" << std::endl;
        stbi_image_free(image);
        return 1;
    }

    // 9. Convert corner to grayscale
    std::vector<unsigned char> grayTL(tlw * tlh);
    for (int y = 0; y < tlh; ++y) {
        for (int x = 0; x < tlw; ++x) {
            int idx = (y * tlw + x) * 3;
            unsigned char r = cornerImg[idx], g = cornerImg[idx + 1], b = cornerImg[idx + 2];
            grayTL[y * tlw + x] = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
        }
    }

    // 10. Binarize corner image
    auto binaryTL = binarize_image(grayTL, tlw, tlh, 100);

    // 11. Find symbol area
    std::vector<Point2f> symbolPoints;
    for (int y = 0; y < tlh; ++y) {
        for (int x = 0; x < tlw; ++x) {
            if (binaryTL[y * tlw + x] == 0) { // znak i broj su crni
                symbolPoints.push_back({(float)x, (float)y});
            }
        }
    }

    if (symbolPoints.empty()) {
        std::cerr << "Nema detektovanih simbola!" << std::endl;
        stbi_image_free(cornerImg);
        stbi_image_free(image);
        return 1;
    }

    // 12. Crop symbol area
    float minX = tlw, minY = tlh, maxX = 0, maxY = 0;
    for (const auto& p : symbolPoints) {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }

    int cropW = (int)(maxX - minX + 1);
    int cropH = (int)(maxY - minY + 1);

    std::vector<unsigned char> finalCrop(cropW * cropH * 3);
    for (int y = 0; y < cropH; ++y) {
        for (int x = 0; x < cropW; ++x) {
            for (int c = 0; c < 3; ++c) {
                finalCrop[(y * cropW + x) * 3 + c] =
                    cornerImg[((int)minY + y) * tlw * 3 + ((int)minX + x) * 3 + c];
            }
        }
    }

    stbi_write_png("step7_symbol_crop.png", cropW, cropH, 3, finalCrop.data(), cropW * 3);
    
    // 13. Split symbol into rank and suit
    split_symbol_image("step7_symbol_crop.png");

    // 14. Load and prepare rank image for matching
    int rank_width, rank_height;
    std::vector<unsigned char> rank_img = load_image_grayscale("broj.png", rank_width, rank_height);
    auto binary_rank = binarize_image(rank_img, rank_width, rank_height, 120);


// 14. Load and prepare rank image for matching
    int suit_width, suit_height;
    std::vector<unsigned char> suit_img = load_image_grayscale("znak.png", suit_width, suit_height);
    auto binary_suit = binarize_image(suit_img, suit_width, suit_height, 120);

    // 15. Match rank
    // U main funkciji, nakon što dobijete binarnu sliku ranka:
int rank = rankMatcher(binary_rank, rank_width, rank_height);
if (rank != -1) {
    std::cout << "Detektovani rank: " << putTextString(rank) << std::endl;
}

   // 16. Match suit
    int suit_code = matchSuit(binary_suit, suit_width, suit_height);
    if (suit_code != -1) {
        std::cout << "Detektovani suit: " << suitToString(suit_code) << std::endl;
    } else {
        std::cout << "Suit nije prepoznat!" << std::endl;
    }

    stbi_image_free(cornerImg);
    stbi_image_free(image);
    
    return 0;
}
