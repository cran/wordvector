/**
 * @file
 * @brief
 * @author Max Fomichev
 * @date 15.02.2017
 * @copyright Apache License v.2 (http://www.apache.org/licenses/LICENSE-2.0)
*/
#include <Rcpp.h>
#include "word2vec.hpp"
#include "trainThread.hpp"

namespace w2v {
    bool word2vec_t::train(const settings_t &_settings,
                           const corpus_t &_corpus) noexcept {
        try {
            
            std::shared_ptr<corpus_t> corpus(new corpus_t(_corpus));
            std::shared_ptr<settings_t> settings(new settings_t(_settings));
            
            m_vectorSize = settings->size;
            m_vocaburarySize = corpus->words.size();
            std::size_t matrixSize = m_vectorSize * m_vocaburarySize;
            std::mt19937_64 randomGenerator(settings->random);
            int iter_max = settings->iterations;
            bool verbose = settings->verbose;
            
            if (m_vectorSize == 0)
                throw std::runtime_error("vectorSize is zero");
                
            if (m_vocaburarySize == 0)
                throw std::runtime_error("vocaburarySize is zero");
            
            if (_corpus.trainWords == 0)
                throw std::runtime_error("trainWords is zero");
            
            // set data
            trainThread_t::data_t data;
            data.settings = settings;
            data.corpus = corpus;
            
            // initialize variables
            data.bpWeights.reset(new std::vector<float>(matrixSize, 0.0f));
            data.pjLayerValues.reset(new std::vector<float>(matrixSize, 0.0f));
            std::uniform_real_distribution<float> rndMatrixInitializer(-0.005f, 0.005f);
            std::generate((*data.pjLayerValues).begin(), (*data.pjLayerValues).end(), [&]() {
                return rndMatrixInitializer(randomGenerator);
            });
            data.expTable.reset(new std::vector<float>(settings->expTableSize));
            for (uint16_t r = 0; r < settings->expTableSize; ++r) {
                // scale value between +- expValueMax
                float s = exp((r / static_cast<float>(settings->expTableSize) * 2.0f - 1.0f) * settings->expValueMax);
                // pre-compute sigmoid: f(x) = exp(x) / (exp(x) + 1)
                (*data.expTable)[r] = s / (s + 1.0f);
                //std::cout << p << "," << s << "," << (*data.expTable)[p] << "\n";
            }
            
            if (settings->withHS) {
                data.huffmanTree.reset(new huffmanTree_t(corpus->frequency));;
            }
            
            data.processedWords.reset(new std::atomic<std::size_t>(0));
            data.alpha.reset(new std::atomic<float>(settings->alpha));
            
            // create threads
            std::vector<std::unique_ptr<trainThread_t>> threads;
            std::size_t n = data.corpus->texts.size();
            std::size_t per = ceil((float)n / (float)data.settings->threads);
            for (std::size_t i = 0; i < settings->threads; ++i) {
                std::size_t from = per * i;
                std::size_t to = std::min(per * (i + 1) - 1, n - 1);
                //Rcpp::Rcout << settings->threads << " " << per << " " << from << " " << to << "\n";
                threads.emplace_back(new trainThread_t(std::make_pair(from, to), data));
                if (n - 1 == to) 
                    break;
            }
            
            int iter = 0;
            float alpha = 0.0;
            for (auto &thread:threads) {
                thread->launch(iter, alpha);
            }
            
            if (verbose) {
                int iter_prev = 0;
                auto start = std::chrono::high_resolution_clock::now();
                while (iter < iter_max) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    if (iter_prev < iter) {
                        auto end = std::chrono::high_resolution_clock::now();
                        auto diff = std::chrono::duration<double, std::milli>(end - start);
                        double msec = diff.count();
                        Rprintf(" ......iteration %d elapsed time: %.2f seconds (alpha: %.4f)\n",
                                iter, msec / 1000, alpha);
                        iter_prev = iter;
                    }
                }
            }
            
            for (auto &thread:threads) {
                thread->join();
            }
            
            // std::cout << "word2vec_t::train()\n";
            // std::cout << data.bpWeights << "\n";
            // for (size_t i = 0; i < 10; i++){
            //     std::cout << (*data.bpWeights)[i] << ", ";
            // }
            // std::cout << "\n";
            
            m_pjLayerValues = *data.pjLayerValues;
            m_bpWeights = *data.bpWeights;
            
            return true;
        } catch (const std::exception &_e) {
            m_errMsg = _e.what();
        } catch (...) {
            m_errMsg = "unknown error";
        }

        return false;
    }
}
