// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "TopologyTraversal.hpp"
#include "GraphObjects/GraphBlock.hpp"
#include "GraphObjects/GraphBreaker.hpp"
#include "GraphObjects/GraphConnection.hpp"

/*!
 * Given an input endpoint, discover all of the "resolved" input endpoints by traversing breakers of the same node name.
 */
static std::vector<GraphConnectionEndpoint> traverseInputEps(const GraphConnectionEndpoint &inputEp, const GraphObjectList &graphObjects)
{
    std::vector<GraphConnectionEndpoint> inputEndpoints;
    auto inputBlock = dynamic_cast<GraphBlock *>(inputEp.getObj().data());
    auto inputBreaker = dynamic_cast<GraphBreaker *>(inputEp.getObj().data());

    if (inputBlock != nullptr)
    {
        inputEndpoints.push_back(inputEp);
    }

    if (inputBreaker != nullptr)
    {
        auto nodeName = inputBreaker->getNodeName();
        for (auto graphObject : graphObjects)
        {
            auto breaker = dynamic_cast<GraphBreaker *>(graphObject);
            if (breaker == nullptr) continue;
            if (breaker->getNodeName() != nodeName) continue;
            if (breaker == inputBreaker) continue;
            //follow all connections from this breaker to an input
            //this is the recursive part
            for (auto graphSubObject : graphObjects)
            {
                auto connection = dynamic_cast<GraphConnection *>(graphSubObject);
                if (connection == nullptr) continue;
                if (connection->getOutputEndpoint().getObj() != breaker) continue;
                for (const auto &epPair : connection->getEndpointPairs())
                {
                    const auto &inputEp = epPair.second;
                    for (const auto &subEp : traverseInputEps(inputEp, graphObjects))
                    {
                        inputEndpoints.push_back(subEp);
                    }
                }
            }
        }
    }

    return inputEndpoints;
}

std::vector<ConnectionInfo> TopologyTraversal::getConnectionInfo(const GraphObjectList &graphObjects)
{
    std::vector<ConnectionInfo> connections;
    for (auto graphObject : graphObjects)
    {
        auto connection = dynamic_cast<GraphConnection *>(graphObject);
        if (connection == nullptr) continue;
        for (const auto &epPair : connection->getEndpointPairs())
        {
            const auto &outputEp = epPair.first;
            const auto &inputEp = epPair.second;

            //ignore connections from output breakers
            //we will come back to them from the block to breaker to block path
            auto outputBreaker = dynamic_cast<GraphBreaker *>(outputEp.getObj().data());
            if (outputBreaker != nullptr) continue;

            for (const auto &subEp : traverseInputEps(inputEp, graphObjects))
            {
                ConnectionInfo info;
                info.srcId = outputEp.getObj()->getId().toStdString();
                info.srcPort = outputEp.getKey().id.toStdString();
                info.dstId = subEp.getObj()->getId().toStdString();
                info.dstPort = subEp.getKey().id.toStdString();
                connections.push_back(info);
            }
        }
    }
    return connections;
}