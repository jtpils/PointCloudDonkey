/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2018, Viktor Seib, Norman Link
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 * * list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/program_options/errors.hpp>
#include "../implicit_shape_model/implicit_shape_model.h"


bool write_log_to_files = true;
bool log_info = true;


int main(int argc, char **argv)
{
    boost::program_options::options_description generic("Generic options");
    boost::program_options::options_description training("Training");
    boost::program_options::options_description detection("Detection");

    generic.add_options()
            ("help,h", "Display this help message")
            ("output,o", boost::program_options::value<std::string>(), "The output folder (created automatically) for ism files after training or the detection log after detection")
            ("inputfile,f", boost::program_options::value<std::string>(), "Input file (for training or testing) containing the input clouds and their corresponding labels (replaces m and c in training and p and g in testing");

    training.add_options()
            ("train,t", boost::program_options::value<std::string>(), "Train an implicit shape model")
            ("inplace,i", "Overwrite the loaded ism file")
            ("models,m", boost::program_options::value<std::vector<std::string> >()->multitoken()->composing(), "Specifiy a list of training models")
            ("classes,c", boost::program_options::value<std::vector<unsigned> >()->multitoken()->composing(), "Specifiy a list of class ids for the given training models");

    detection.add_options()
            ("detect,d", boost::program_options::value<std::string>(), "Detect using a trained implicit shape model")
            ("pointclouds,p", boost::program_options::value<std::vector<std::string> >()->multitoken()->composing(), "Specify a list of input point clouds")
            ("groundtruth,g", boost::program_options::value<std::vector<unsigned> >()->multitoken()->composing(), "Specifiy a list of ground truth class ids for the given pointclouds");


    boost::program_options::options_description desc;
    desc.add(generic).add(training).add(detection);

    // parse command line arguments
    boost::program_options::variables_map variables;
    try {
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).run(), variables);
        boost::program_options::notify(variables);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    // show help
    if (variables.count("help") || variables.size() == 0) {
        std::cout << desc << std::endl;
        return 1;
    }
    else {

        std::vector<std::string> filenames;
        std::vector<unsigned> labels;
        std::string mode = "";

        if(variables.count("inputfile"))
        {
            // manually parse input file if available, then adapt it to boost params
            std::string input_file = variables["inputfile"].as<std::string>();

            // parse input
            std::ifstream infile(input_file);
            std::string file;
            std::string label;

            while(infile >> file >> label)
            {
                // special treatment of first line: determine mode
                if(file == "#" && (label == "train" || label == "test"))
                {
                    mode = label;
                }
                else // other lines contain a filename and a label
                {
                    filenames.push_back(file);
                    labels.push_back(std::stoul(label));
                }
            }
        }

        try {
            // train the ISM
            if ((variables.count("train") && mode == "") || mode == "train")
            {
                std::cout << "starting the training process" << std::endl;

                std::string ismFile;
                try // allows to use -t or -d for ism-files when input file with dataset is specified with -f
                {
                    ismFile = variables["train"].as<std::string>();
                }
                catch(std::exception& e)
                {
                    ismFile = variables["detect"].as<std::string>();
                }

                // try to read the file
                ism3d::ImplicitShapeModel ism;
                ism.setLogging(log_info);
                ism.setSignalsState(false); // disable signals since we are using command line, no GUI

                if (!ism.readObject(ismFile, true))
                {
                    std::cerr << "could not read ism from file, training stopped: " << ismFile << std::endl;
                    return 1;
                }

                // set output filename
                if (variables.count("output"))
                {
                    std::string outFilename = variables["output"].as<std::string>();
                    ism.setOutputFilename(outFilename);
                }

                // add the training models to the ism
                if ((variables.count("models") && variables.count("classes")) ||
                        (filenames.size() > 0 && labels.size() > 0))
                {
                    std::vector<std::string> models;
                    std::vector<unsigned> classIds;

                    if(variables.count("models")) // input directly from command line
                    {
                        models = variables["models"].as<std::vector<std::string> >();
                        classIds = variables["classes"].as<std::vector<unsigned> >();
                    }
                    else if(filenames.size() > 0) // input inside file given on command line
                    {
                        models = filenames;
                        classIds = labels;
                    }

                    if (models.size() == classIds.size())
                    {
                        for (int i = 0; i < (int)models.size(); i++)
                        {
                            std::string filename = models[i];
                            unsigned classId = classIds[i];

                            // add the training model to the ISM
                            if (!ism.addTrainingModel(filename, classId))
                            {
                                std::cerr << "could not add training model: " << filename << ", class " << classId << std::endl;
                                return 1;
                            }
                        }
                    }
                    else
                    {
                        std::cerr << "number of models does not match the number of class ids" << std::endl;
                        return 1;
                    }
                }

                // train
                ism.train();

                // write the ism data
                if (variables.count("inplace"))
                {
                    if (!ism.writeObject(ismFile, ismFile + "d"))
                    {
                        std::cerr << "could not write ism" << std::endl;
                        return 1;
                    }
                }
                else if (variables.count("output"))
                {
                    std::string outFilename = variables["output"].as<std::string>();
                    if (!ism.writeObject(outFilename))
                    {
                        std::cerr << "could not write ism" << std::endl;
                        return 1;
                    }
                }
                else
                {
                    std::cerr << "the trained ism is not saved" << std::endl;
                    return 1;
                }
            }

            // detect the ISM
            if ((variables.count("detect") && mode == "") || mode == "test")
            {
                std::cout << "starting the detection process" << std::endl;

                std::string ismFile;
                try // allows to use -t or -d for ism-files when input file with dataset is specified with -f
                {
                    ismFile = variables["detect"].as<std::string>();
                }
                catch(std::exception& e)
                {
                    ismFile = variables["train"].as<std::string>();
                }

                // try to read the file
                ism3d::ImplicitShapeModel ism;
                ism.setLogging(log_info);
                ism.setSignalsState(false); // disable signals since we are using command line, no GUI

                if (!ism.readObject(ismFile))
                {
                    std::cerr << "could not read ism from file, detection stopped: " << ismFile << std::endl;
                    return 1;
                }
                else if ((variables.count("pointclouds") && variables.count("groundtruth")) ||
                         (filenames.size() > 0 && labels.size() > 0))  // load pointclouds and groundtruth
                {
                    std::vector<std::string> pointClouds;
                    std::vector<unsigned> groundtruth;

                    if(variables.count("pointclouds")) // input directly from command line
                    {
                        pointClouds = variables["pointclouds"].as<std::vector<std::string> >();
                        groundtruth = variables["groundtruth"].as<std::vector<unsigned> >();
                    }
                    else if(filenames.size() > 0) // input inside file given on command line
                    {
                        pointClouds = filenames;
                        groundtruth = labels;
                    }

                    // prepare summary
                    std::ofstream summaryFile;
                    int numCorrectClasses = 0;
                    int numCorrect80 = 0;

                    int numCorrectGlobal = 0;
                    int numBothCorrect = 0;
                    int numOnlyGlobalCorrect = 0;

                    //std::cout << "preparing output folder" << std::endl;

                    if (variables.count("output"))
                    {
                        // create folder for output
                        std::string command = "mkdir ";
                        std::string folder = variables["output"].as<std::string>();
                        command.append(folder);
                        int unused = std::system(command.c_str());
                        sleep(1);

                        // summary file
                        std::string outFile = variables["output"].as<std::string>();
                        std::string outFileName = outFile;
                        outFileName.append("/summary.txt");
                        summaryFile.open(outFileName.c_str(), std::ios::out);
                    }
                    else
                    {
                        std::cerr << "no output file specified, detected maxima will not be saved" << std::endl;
                    }

                    if (pointClouds.size() == groundtruth.size())
                    {
                        boost::timer::cpu_timer timer;

                        std::map<std::string, double> times;
                        for(unsigned i = 0; i < pointClouds.size(); i++)
                        {
                            // detect
                            std::string pointCloud = pointClouds.at(i);
                            unsigned trueID = groundtruth.at(i);
                            std::vector<ism3d::VotingMaximum> maxima;

                            std::cout << "Processing file: " << pointCloud << std::endl;
                            if (!ism.detect(pointCloud, maxima, times))
                            {
                                std::cerr << "detection failed" << std::endl;
                                return 1;
                            }
                            else
                            {
                                //std::cout << "detected " << maxima.size() << " maxima" << std::endl;

                                // write detected maxima to detection log file
                                if (variables.count("output"))
                                {
                                    if(write_log_to_files)
                                    {
                                        unsigned tmp = pointCloud.find_last_of('/');
                                        if(tmp == std::string::npos) tmp = 0;
                                        std::string fileWithoutFolder = pointCloud.substr(tmp+1);

                                        std::cout << "writing detection log" << std::endl;
                                        std::string outFile = variables["output"].as<std::string>();
                                        std::string outFileName = outFile;
                                        outFileName.append("/");
                                        outFileName.append(fileWithoutFolder);
                                        outFileName.append(".txt");

                                        std::ofstream file;
                                        file.open(outFileName.c_str(), std::ios::out);
                                        file << "ISM3D detection log, filename: " << ismFile << ", point cloud: " << pointCloud << ", ground truth class ID: " << trueID << "\n";
                                        file << "number, classID, weight, num-votes, position X Y Z, bounding box size X Y Z, bounding Box rotation quaternion w x y z \n";

                                        for (int i = 0; i < (int)maxima.size(); i++)
                                        {
                                            const ism3d::VotingMaximum& maximum = maxima[i];

                                            file << i << ", ";
                                            file << maximum.classId << ", ";
                                            file << maximum.weight << ", ";
                                            file << maximum.voteIndices.size() << ", ";
                                            file << maximum.position[0] << ", ";
                                            file << maximum.position[1] << ", ";
                                            file << maximum.position[2] << ", ";
                                            file << maximum.boundingBox.size[0] << ", ";
                                            file << maximum.boundingBox.size[1] << ", ";
                                            file << maximum.boundingBox.size[2] << ", ";
                                            file << maximum.boundingBox.rotQuat.R_component_1() << ", ";
                                            file << maximum.boundingBox.rotQuat.R_component_2() << ", ";
                                            file << maximum.boundingBox.rotQuat.R_component_3() << ", ";
                                            file << maximum.boundingBox.rotQuat.R_component_4() << std::endl;
                                        }

                                        file.close();
                                    }

                                    // writing summary file
                                    int classId = -1;
                                    int classIdglobal = -1;
                                    int classId80 = -1;
                                    if(maxima.size() > 0)
                                    {
                                        classId = maxima.at(0).classId;
                                        classIdglobal = maxima.at(0).globalHypothesis.first;
                                    }

                                    // check for classification including non-best maxima
                                    for (int i = 0; i < (int)maxima.size(); i++)
                                    {
                                        const ism3d::VotingMaximum& maximum = maxima[i];
                                        if(maximum.weight < maxima[0].weight * 0.8) break;

                                        classId80 = maximum.classId;
                                        if(classId80 == trueID) break;
                                    }


                                    // only display additional classifiers if they are different from normal classification
                                    summaryFile << "file: " << pointCloud << ", ground truth class: " << trueID << ", classified class: " << classId;

                                    if(classId != classIdglobal)
                                    {
                                        summaryFile << ", global class: " << classIdglobal;
                                    }
                                    summaryFile << std::endl;

                                    // count correct matches
                                    // normal classifier
                                    if(((int)trueID) == classId)
                                    {
                                        numCorrectClasses++;
                                    }
                                    if(((int)trueID) == classId80)
                                    {
                                        numCorrect80++;
                                    }
                                    // global classifier
                                    if(((int)trueID) == classIdglobal)
                                    {
                                        numCorrectGlobal++;
                                    }
                                    // both correct
                                    if((int)trueID == classId && (int)trueID == classIdglobal)
                                    {
                                        numBothCorrect++;
                                    }
                                    // global correct, normal wrong
                                    if((int)trueID != classId && (int)trueID == classIdglobal)
                                    {
                                        numOnlyGlobalCorrect++;
                                    }
                                }
                            }
                        }

                        // write processing time details to summary
                        double time_sum = 0;
                        for(auto it : times)
                        {
                            if(it.first == "complete") continue;
                            time_sum += (it.second / 1000);
                        }
                        summaryFile << "\n\n\ncomplete time: " << times["complete"] / 1000 << " [s]" << ", sum all steps: " << time_sum << " [s]" << std::endl;
                        summaryFile << "times per step:\n";
                        summaryFile << "create flann index: " << std::setw(10) << std::setfill(' ') << times["flann"] / 1000 << " [s]" << std::endl;
                        summaryFile << "compute normals:    " << std::setw(10) << std::setfill(' ') << times["normals"] / 1000 << " [s]" << std::endl;
                        summaryFile << "compute keypoints:  " << std::setw(10) << std::setfill(' ') << times["keypoints"] / 1000 << " [s]" << std::endl;
                        summaryFile << "compute features:   " << std::setw(10) << std::setfill(' ') << times["features"] / 1000 << " [s]" << std::endl;
                        summaryFile << "cast votes:         " << std::setw(10) << std::setfill(' ') << times["voting"] / 1000 << " [s]" << std::endl;
                        summaryFile << "find maxima:        " << std::setw(10) << std::setfill(' ') << times["maxima"] / 1000 << " [s]" << std::endl;

                        // complete and close summary file
                        summaryFile << "\n\n result: " << numCorrectClasses << " of " << pointClouds.size() << " shapes classified correctly ("
                                    << ((float)numCorrectClasses/pointClouds.size())*100.0f << " %)\n";
                        summaryFile << " result: " << numCorrect80 << " of " << pointClouds.size() << " shapes classified correctly ("
                                    << ((float)numCorrect80/pointClouds.size())*100.0f << " %) [above 80% of top result's score]\n";

                        summaryFile << " result: " << numCorrectGlobal << " of " << pointClouds.size() << " shapes classified correctly with global descriptors ("
                                    << ((float)numCorrectGlobal/pointClouds.size())*100.0f << " %)\n\n";
                        summaryFile << " both correct: " << numBothCorrect << " (" << ((float)numBothCorrect/pointClouds.size())*100.0f << " %)\n";
                        summaryFile << " only global correct: " << numOnlyGlobalCorrect << " (" << ((float)numOnlyGlobalCorrect/pointClouds.size())*100.0f << " %)\n\n\n";

                        summaryFile << " Total processing time: " << timer.format(4, "%w") << " seconds \n";
                        summaryFile.close();

                    }
                    else
                    {
                        std::cerr << "number of pointclouds does not match the number of groundtruth ids" << std::endl;
                        return 1;
                    }
                }
                else
                {
                    std::cerr << "number of pointclouds arguments does not match the number of groundtruth id arguments" << std::endl;
                    return 1;
                }
            }
        }
        catch (const ism3d::Exception& e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }
        catch (...) {
            std::cerr << "an exception occurred" << std::endl;
            return 1;
        }
    }

    return 0;
}
