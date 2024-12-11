/**
 * @file
 * @brief trainThread trains a word2vec model from the specified part of train data set file
 * @author Max Fomichev
 * @date 20.12.2016
 * @copyright Apache License v.2 (http://www.apache.org/licenses/LICENSE-2.0)
*/

#ifndef WORD2VEC_TRAINTHREAD_H
#define WORD2VEC_TRAINTHREAD_H

#include <memory>
#include <random>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <stdexcept>

#include "word2vec.hpp"
#include "huffmanTree.hpp"
#include "nsDistribution.hpp"
#include "downSampling.hpp"

namespace w2v {
    /**
     * @brief trainThread class - train thread and its local data
     *
     *  trainThread class trains a word2vec model from the specified part of train data set file.
     *  Here are two supported training model algorithms - CBOW and Skip-Gram and two approximation algorithms to
     *  speedup training - Hierarchical Softmax (HS) and Negative Sampling (NS).
     *  It is possible to choose any of the following algorithms combination - CBOW/HS or CBOW/NS or Skip-Gram/HS or
     *  Skip-Gram/NS.
    */
    class trainThread_t final {
    public:
        /**
         * @brief data structure holds all common data used by train threads
        */
        struct data_t final {
            std::shared_ptr<settings_t> settings; ///< settings structure
            //std::shared_ptr<vocabulary_t> vocabulary; ///< words data
            std::shared_ptr<corpus_t> corpus; ///< train data 
            //std::shared_ptr<fileMapper_t> fileMapper; /// NOTE: remove
            std::shared_ptr<std::vector<float>> bpWeights; ///< back propagation weights
            std::shared_ptr<std::vector<float>> expTable; ///< exp(x) / (exp(x) + 1) values lookup table
            std::shared_ptr<huffmanTree_t> huffmanTree; ///< Huffman tree used by hierarchical softmax
            std::shared_ptr<std::atomic<std::size_t>> processedWords; ///< total words processed by train threads
            std::shared_ptr<std::atomic<float>> alpha; ///< current learning rate
            std::function<void(float, float)> progressCallback = nullptr; ///< callback with alpha and training percent
        };
        
        // NOTE: used for corpus
        std::pair<std::size_t, std::size_t> range;
        
    private:
        data_t m_data;
        
        std::random_device m_randomDevice;
        std::mt19937_64 m_randomGenerator;
        std::uniform_int_distribution<short> m_rndWindowShift;
        std::uniform_int_distribution<short> m_rndWindow;
        std::unique_ptr<downSampling_t> m_downSampling;
        std::unique_ptr<nsDistribution_t> m_nsDistribution;
        std::unique_ptr<std::vector<float>> m_hiddenLayerVals;
        std::unique_ptr<std::vector<float>> m_hiddenLayerErrors;
        std::unique_ptr<std::thread> m_thread;

    public:
        /**
         * Constructs train thread local data
         * @param _id thread ID, starting from 0
         * @param _data data object instantiated outside of the thread
        */
        trainThread_t(uint16_t _id, const data_t &_data);

        /**
         * Launchs the thread
         * @param[out] _trainMatrix - train model matrix
        */
        void launch(std::vector<float> &_trainMatrix) noexcept {
            m_thread.reset(new std::thread(&trainThread_t::worker, this, std::ref(_trainMatrix)));
        }
        /// Joins to the thread
        void join() noexcept {
            return m_thread->join();
        }

    private:
        void worker(std::vector<float> &_trainMatrix) noexcept;

        inline void cbow(const std::vector<unsigned int> &_sentence,
                         std::vector<float> &_trainMatrix) noexcept;
        inline void cbowOld(const std::vector<unsigned int> &_sentence,
                         std::vector<float> &_trainMatrix) noexcept;
        inline void skipGram(const std::vector<unsigned int> &_sentence,
                             std::vector<float> &_trainMatrix) noexcept;
        inline void skipGramOld(const std::vector<unsigned int> &_sentence,
                             std::vector<float> &_trainMatrix) noexcept;
        inline void hierarchicalSoftmax(std::size_t _index,
                                        std::vector<float> &_hiddenLayer,
                                        std::vector<float> &_trainLayer, std::size_t _trainLayerShift) noexcept;
        inline void negativeSampling(std::size_t _index,
                                     std::vector<float> &_hiddenLayer,
                                     std::vector<float> &_trainLayer, std::size_t _trainLayerShift) noexcept;
    };

}

#endif //WORD2VEC_TRAINTHREAD_H