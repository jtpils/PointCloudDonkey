/*
 * BSD 3-Clause License
 *
 * Full text: https://opensource.org/licenses/BSD-3-Clause
 *
 * Copyright (c) 2018, Viktor Seib
 * All rights reserved.
 *
 */

#ifndef ISM3D_CODEWORD_H
#define ISM3D_CODEWORD_H

#include "../utils/json_object.h"

#include <vector>
#include <fstream>
#include <Eigen/Core>
#include <boost/shared_ptr.hpp>

namespace ism3d
{
    /**
     * @brief The Codeword class
     * A codeword is a geometric pattern on an object. Is is computed by clustering similar
     * descriptors.
     */
    class Codeword
            : public JSONObject
    {
    public:
        Codeword();
        ~Codeword();

        /**
         * @brief Create the codeword with the specified data vector.
         * @param data the data vector
         * @param numFeatures the number of features from which the codeword was learned
         * @param weight computed weight of the descriptor that represents the codeword
         */
        Codeword(const std::vector<float>& data, int numFeatures, float weight);

        /**
         * @brief Set or change the data vector.
         * @param data the data vector
         * @param numFeatures the number of features from which the codeword was learned
         * @param weight computed weight of the descriptor that represents the codeword
         */
        void setData(const std::vector<float>& data, int numFeatures, float weight);

        /**
         * @brief Get the data vector for this codeword.
         * @return the data vector
         */
        const std::vector<float>& getData() const;

        /**
         * @brief Get the codeword id.
         * @return the codeword id
         */
        int getId() const;

        /**
         * @brief Get the number of features from which the codeword was learned.
         * @return the number of features from which the codeword was learned
         */
        int getNumFeatures() const;

        float getWeight() const;

        /**
         * @brief add a feature position to this codeword when it has been activated
         * @param featurePos the feature position that activated this codeword
         * @param classId the class id of the feature
         * @param weight weight of the descriptor that this featurePos belongs to
         */
        void addFeature(const Eigen::Vector3f& featurePos, unsigned classId);

        const std::vector<Eigen::Vector3f>& getFeaturePositions() const;
        const std::vector<unsigned>& getFeatureClasses() const;

    protected:

        void iSaveData(boost::archive::binary_oarchive &oa) const;
        bool iLoadData(boost::archive::binary_iarchive &ia);

        Json::Value iDataToJson() const;
        bool iDataFromJson(const Json::Value&);

    private:
        static int m_maxId;
        int m_id;
        float m_weight; // descriptor weight
        std::vector<float> m_data; // this is the descriptor
        int m_numFeatures;
        std::vector<unsigned> m_featureClasses;

        // TEMP
        std::vector<Eigen::Vector3f> m_featurePositions;
    };
}

#endif // ISM3D_CODEWORD_H
