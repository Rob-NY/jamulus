/******************************************************************************\
 * Copyright (c) 2021-2022
 *
 * Author(s):
 *  dtinth
 *  Christian Hoffmann
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 \******************************************************************************/

#include "serverrpc.h"

CServerRpc::CServerRpc ( CServer* pServer, CRpcServer* pRpcServer, QObject* parent ) : QObject ( parent )
{
    // API doc already part of CClientRpc
    pRpcServer->HandleMethod ( "jamulus/getMode", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonObject result{ { "mode", "server" } };
        response["result"] = result;
        Q_UNUSED ( params );
    } );

#ifndef NO_FIREWALL
    /// @rpc_method jamulusserver/addFirewallAddress
    /// @brief Adds an address to the internal access control list.
    /// @param {string} params.address - The address to add to the access control list.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusserver/addFirewallAddress", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonAddress = params["address"];
        if ( !jsonAddress.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: address is not a string" );
            return;
        }

        pServer->GetSocketObject()->fwAdd ( QString ( jsonAddress.toString() ) );
        response["result"] = "ok";
    } );

    /// @rpc_method jamulusserver/addFirewallAddresses
    /// @brief Adds multiple address to the internal access control list.
    /// @param {array} params.addresses - The addresses to add to the access control list.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusserver/addFirewallAddresses", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonAddresses = params["addresses"];

        if ( !jsonAddresses.isArray() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: addresses must be an array" );
            return;
        }

        for ( auto addr : jsonAddresses.toArray() )
        {
            if ( !addr.isString() )
            {
                response["error"] =
                    CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: address within array is not a string" );
                return;
            }

            pServer->GetSocketObject()->fwAdd ( QString ( addr.toString() ) );
        }

        response["result"] = "ok";
    } );

    /// @rpc_method jamulusserver/removeFirewallAddress
    /// @brief Remove an address from the internal access control list.
    /// @param {string} params.address - The address to remove from the access control list.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusserver/removeFirewallAddress", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonAddress = params["address"];
        if ( !jsonAddress.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: address is not a string" );
            return;
        }

        pServer->GetSocketObject()->fwRemove ( QString ( jsonAddress.toString() ) );
        response["result"] = "ok";
    } );

    /// @rpc_method jamulusserver/setFirewallMode
    /// @brief Sets the access control mode (open or closed)
    /// @param {int} params.mode - Sets the access control mode 0=Open, 1=Closed.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusserver/setFirewallMode", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonMode = params["mode"];
        if ( !jsonMode.isDouble() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: mode must be numeric" );
            return;
        }

        int mode = jsonMode.toInt();

        if ( mode != 0 && mode != 1 )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: mode must be 0 or 1" );
            return;
        }

        pServer->GetSocketObject()->fwSetMode ( mode );
        response["result"] = "ok";
    } );

    /// @rpc_method jamulusserver/resetFirewall
    /// @brief Reset the access control mode to OPEN and removes all addresses from control list.
    /// @param {string} params - No parameters (empty object).
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusserver/resetFirewall", [=] ( const QJsonObject& params, QJsonObject& response ) {
        pServer->GetSocketObject()->fwReset();
        response["result"] = "ok";

        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/getFirewallStatus
    /// @brief Returns the status of the firewall and control list.
    /// @param {string} params - No parameters (empty object).
    /// @result {int} result.mode - The current mode; 0=Open, 1=Closed.
    /// @result {array} result.addresses - Addresses on the control list as an array.
    pRpcServer->HandleMethod ( "jamulusserver/getFirewallStatus", [=] ( const QJsonObject& params, QJsonObject& response ) {
        int mode = pServer->GetSocketObject()->fwGetMode();

        QStringList ips;
        pServer->GetSocketObject()->fwGetAddresses ( ips );

        QJsonArray addresses = QJsonArray::fromStringList ( ips );

        QJsonObject result{
            { "mode", mode },
            { "addresses", addresses },
        };

        response["result"] = result;

        Q_UNUSED ( params );
    } );

#endif
    /// @rpc_method jamulusserver/getRecorderStatus
    /// @brief Returns the recorder state.
    /// @param {object} params - No parameters (empty object).
    /// @result {boolean} result.initialised - True if the recorder is initialised.
    /// @result {string} result.errorMessage - The recorder error message, if any.
    /// @result {boolean} result.enabled - True if the recorder is enabled.
    /// @result {string} result.recordingDirectory - The recorder recording directory.
    pRpcServer->HandleMethod ( "jamulusserver/getRecorderStatus", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonObject result{
            { "initialised", pServer->GetRecorderInitialised() },
            { "errorMessage", pServer->GetRecorderErrMsg() },
            { "enabled", pServer->GetRecordingEnabled() },
            { "recordingDirectory", pServer->GetRecordingDir() },
        };

        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/getClients
    /// @brief Returns the list of connected clients along with details about them.
    /// @param {object} params - No parameters (empty object).
    /// @result {number} result.connections - The number of active connections.
    /// @result {array}  result.clients - The list of connected clients.
    /// @result {number} result.clients[*].id - The client’s channel id.
    /// @result {string} result.clients[*].address - The client’s address (ip:port).
    /// @result {string} result.clients[*].name - The client’s name.
    /// @result {number} result.clients[*].jitterBufferSize - The client’s jitter buffer size.
    /// @result {number} result.clients[*].channels - The number of audio channels of the client.
    /// @result {number} result.clients[*].instrumentCode - The id of the instrument for this channel.
    /// @result {number} result.clients[*].instrumentName - The text name of the instrument for this channel.
    /// @result {string} result.clients[*].city - The city name provided by the user for this channel.
    /// @result {number} result.clients[*].countryCode - The id of the country specified by the user for this channel (see QLocale::Country).
    /// @result {number} result.clients[*].countryName - The text name of the country specified by the user for this channel (see QLocale::Country).
    /// @result {number} result.clients[*].skillLevelCode - The skill level id provided by the user for this channel.
    /// @result {number} result.clients[*].skillLevelName - The skill level text name provided by the user for this channel.
    pRpcServer->HandleMethod ( "jamulusserver/getClients", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonArray                clients;
        CVector<CHostAddress>     vecHostAddresses;
        CVector<QString>          vecsName;
        CVector<int>              veciJitBufNumFrames;
        CVector<int>              veciNetwFrameSizeFact;
        CVector<CChannelCoreInfo> vecChanInfo;

        int connections = 0;

        pServer->GetConCliParam ( vecHostAddresses, vecsName, veciJitBufNumFrames, veciNetwFrameSizeFact, vecChanInfo );

        // we assume that all vectors have the same length
        const int iNumChannels = vecHostAddresses.Size();

        // fill list with connected clients
        for ( int i = 0; i < iNumChannels; i++ )
        {
            if ( vecHostAddresses[i].InetAddr == QHostAddress ( static_cast<quint32> ( 0 ) ) )
            {
                continue;
            }

            QJsonObject client{
                { "id", i },
                { "address", vecHostAddresses[i].toString ( CHostAddress::SM_IP_PORT ) },
                { "name", vecsName[i] },
                { "jitterBufferSize", veciJitBufNumFrames[i] },
                { "channels", pServer->GetClientNumAudioChannels ( i ) },
                { "instrumentCode", vecChanInfo[i].iInstrument },
                { "instrumentName", CInstPictures::GetName ( vecChanInfo[i].iInstrument ) },
                { "city", vecChanInfo[i].strCity },
                { "countryCode", vecChanInfo[i].eCountry },
                { "countryName", QLocale::countryToString ( vecChanInfo[i].eCountry ) },
                { "skillLevelCode", vecChanInfo[i].eSkillLevel },
                { "skillLevelName", SkillLevelToString ( vecChanInfo[i].eSkillLevel ) },
            };
            clients.append ( client );

            ++connections;
        }

        // create result object
        QJsonObject result{
            { "connections", connections },
            { "clients", clients },
        };
        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/getServerProfile
    /// @brief Returns the server registration profile and status.
    /// @param {object} params - No parameters (empty object).
    /// @result {string} result.name - The server name.
    /// @result {string} result.city - The server city.
    /// @result {number} result.countryId - The server country ID (see QLocale::Country).
    /// @result {string} result.welcomeMessage - The server welcome message.
    /// @result {string} result.directoryServer - The directory server to which this server requested registration, or blank if none.
    /// @result {string} result.registrationStatus - The server registration status as string (see ESvrRegStatus and SerializeRegistrationStatus).
    pRpcServer->HandleMethod ( "jamulusserver/getServerProfile", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QString dsName = "";

        if ( AT_NONE != pServer->GetDirectoryType() )
            dsName = NetworkUtil::GetDirectoryAddress ( pServer->GetDirectoryType(), pServer->GetDirectoryAddress() );

        QJsonObject result{
            { "name", pServer->GetServerName() },
            { "city", pServer->GetServerCity() },
            { "countryId", pServer->GetServerCountry() },
            { "welcomeMessage", pServer->GetWelcomeMessage() },
            { "directoryServer", dsName },
            { "registrationStatus", SerializeRegistrationStatus ( pServer->GetSvrRegStatus() ) },
        };
        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/setServerName
    /// @brief Sets the server name.
    /// @param {string} params.serverName - The new server name.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusserver/setServerName", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonServerName = params["serverName"];
        if ( !jsonServerName.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: serverName is not a string" );
            return;
        }

        pServer->SetServerName ( jsonServerName.toString() );
        response["result"] = "ok";
    } );

    /// @rpc_method jamulusserver/setWelcomeMessage
    /// @brief Sets the server welcome message.
    /// @param {string} params.welcomeMessage - The new welcome message.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusserver/setWelcomeMessage", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonWelcomeMessage = params["welcomeMessage"];
        if ( !jsonWelcomeMessage.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: welcomeMessage is not a string" );
            return;
        }

        pServer->SetWelcomeMessage ( jsonWelcomeMessage.toString() );
        response["result"] = "ok";
    } );

    /// @rpc_method jamulusserver/setRecordingDirectory
    /// @brief Sets the server recording directory.
    /// @param {string} params.recordingDirectory - The new recording directory.
    /// @result {string} result - Always "acknowledged".
    ///  To check if the directory was changed, call `jamulusserver/getRecorderStatus` again.
    pRpcServer->HandleMethod ( "jamulusserver/setRecordingDirectory", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonRecordingDirectory = params["recordingDirectory"];
        if ( !jsonRecordingDirectory.isString() )
        {
            response["error"] =
                CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: recordingDirectory is not a string" );
            return;
        }

        pServer->SetRecordingDir ( jsonRecordingDirectory.toString() );
        response["result"] = "acknowledged";
    } );

    /// @rpc_method jamulusserver/startRecording
    /// @brief Starts the server recording.
    /// @param {object} params - No parameters (empty object).
    /// @result {string} result - Always "acknowledged".
    ///  To check if the recording was enabled, call `jamulusserver/getRecorderStatus` again.
    pRpcServer->HandleMethod ( "jamulusserver/startRecording", [=] ( const QJsonObject& params, QJsonObject& response ) {
        pServer->SetEnableRecording ( true );
        response["result"] = "acknowledged";
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/stopRecording
    /// @brief Stops the server recording.
    /// @param {object} params - No parameters (empty object).
    /// @result {string} result - Always "acknowledged".
    ///  To check if the recording was disabled, call `jamulusserver/getRecorderStatus` again.
    pRpcServer->HandleMethod ( "jamulusserver/stopRecording", [=] ( const QJsonObject& params, QJsonObject& response ) {
        pServer->SetEnableRecording ( false );
        response["result"] = "acknowledged";
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/restartRecording
    /// @brief Restarts the recording into a new directory.
    /// @param {object} params - No parameters (empty object).
    /// @result {string} result - Always "acknowledged".
    ///  To check if the recording was restarted or if there is any error, call `jamulusserver/getRecorderStatus` again.
    pRpcServer->HandleMethod ( "jamulusserver/restartRecording", [=] ( const QJsonObject& params, QJsonObject& response ) {
        pServer->RequestNewRecording();
        response["result"] = "acknowledged";
        Q_UNUSED ( params );
    } );
}

QJsonValue CServerRpc::SerializeRegistrationStatus ( ESvrRegStatus eSvrRegStatus )
{
    switch ( eSvrRegStatus )
    {
    case SRS_NOT_REGISTERED:
        return "not_registered";

    case SRS_BAD_ADDRESS:
        return "bad_address";

    case SRS_REQUESTED:
        return "requested";

    case SRS_TIME_OUT:
        return "time_out";

    case SRS_UNKNOWN_RESP:
        return "unknown_resp";

    case SRS_REGISTERED:
        return "registered";

    case SRS_SERVER_LIST_FULL:
        return "directory_server_full";

    case SRS_VERSION_TOO_OLD:
        return "server_version_too_old";

    case SRS_NOT_FULFILL_REQUIREMENTS:
        return "requirements_not_fulfilled";
    }

    return QString ( "unknown(%1)" ).arg ( eSvrRegStatus );
}
