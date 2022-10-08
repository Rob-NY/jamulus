/******************************************************************************\
 * Copyright (c) 2004-2022
 *
 * Author(s):
 *  Volker Fischer
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

#include "channel.h"
#include "serverlogging.h"

// Server logging --------------------------------------------------------------

CServerLogging::CServerLogging() : bDoLogging ( false ), File ( DEFAULT_LOG_FILE_NAME ) {}

CServerLogging::~CServerLogging()
{
    // close logging file of open
    if ( File.isOpen() )
    {
        File.close();
    }
}

void CServerLogging::Start ( const QString& strLoggingFileName )
{
    // open file
    File.setFileName ( strLoggingFileName );

    if ( File.open ( QIODevice::Append | QIODevice::Text ) )
    {
        bDoLogging = true;
    }
}

void CServerLogging::AddNewConnection ( const CHostAddress& ClientInetAddr, const int iNumberOfConnectedClients )
{

    //
    // CHostAddress has both IP and port, so we'll display both
    //

    // logging of new connected channel
    const QString strLogStr =
        CurTimeDatetoLogString() + "\tCONNECT\t" + ClientInetAddr.toString() + "\tconnected (" + QString::number ( iNumberOfConnectedClients ) + ")";

    qInfo() << qUtf8Printable ( strLogStr ); // on console
    *this << strLogStr;                      // in log file
}

void CServerLogging::AddServerStopped()
{

    const QString strLogStr = CurTimeDatetoLogString() + "\tIDLE";

    qInfo() << qUtf8Printable ( strLogStr ); // on console
    *this << strLogStr;                      // in log file
}

void CServerLogging::AddChannelInfoChanged ( CChannel* channel )
{
    // We're not going to print these to the console unless logging is enabled, so we can short-circuit this
    // method here to save some cycles if logging is not enabled.

    if ( !bDoLogging )
    {
        return;
    }

    //
    // CHostAddr has both address and port and we will use that here just as in connect.
    //

    auto address_parts = channel->GetAddress().toString();

    //
    // Sanitize the channel name to remove tab characters, newlines, etc. for TSV processing
    //

    QString cName = channel->GetName();

    cName.replace ( QString ( "\n" ), QString ( " " ) );
    cName.replace ( QString ( "\r" ), QString ( " " ) );
    cName.replace ( QString ( "\t" ), QString ( " " ) );
    cName.replace ( QString ( "\\" ), QString ( "\\\\" ) );
    cName.replace ( QString ( "\"" ), QString ( "\\\"" ) );

    // qDebug() << "Channel input changed " << channel->GetName() << " - " << channel->GetAddress().toString();

    const QString strLogStr = CurTimeDatetoLogString() + "\tCHANNEL\t" + address_parts + "\t" + cName;
    qInfo() << qUtf8Printable ( strLogStr ); // on console
    *this << strLogStr;                      // in log file
}

void CServerLogging::operator<< ( const QString& sNewStr )
{
    if ( bDoLogging )
    {
        // append new line in logging file
        QTextStream out ( &File );
        out << sNewStr << '\n';
        out.flush();
    }
}

QString CServerLogging::CurTimeDatetoLogString()
{
    // time and date to string conversion
    const QDateTime curDateTime = QDateTime::currentDateTime();

    // format date and time output according to "2006-09-30 11:38:08"
    return curDateTime.toString ( "yyyy-MM-dd HH:mm:ss" );
}
