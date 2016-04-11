/**
 * Copyright (C) 2016 Martin Ubl <http://pivo.kennny.cz>
 *
 * This file is part of PIVO html output module.
 *
 * PIVO html output module is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * PIVO html output module is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PIVO html output module. If not,
 * see <http://www.gnu.org/licenses/>.
 **/

#ifndef PIVO_HTML_BUFFERED_READER_H
#define PIVO_HTML_BUFFERED_READER_H

// default size of BufferedReader buffer
#define BUFFERED_READER_DEFAULT_BUF_SIZE 64

#include <stack>

class BufferedReader
{
    public:
        BufferedReader(FILE* f, int bufferLength = BUFFERED_READER_DEFAULT_BUF_SIZE);
        ~BufferedReader();

        // Close file if any
        void Close();

        // Is end of file reached?
        bool IsEof();
        // reads character from opened file
        bool ReadChar(char &c);
        // returns character temporarily back
        void ReturnChar(char c);

    protected:

        // reads buffer from file
        bool FillBuffer();

        // file we are reading now
        FILE* m_file;
        // buffer of input characters
        char* m_buffer;
        // buffer length used
        int m_bufferLength;
        // current read position in buffer
        int m_bufferPos;
        // temporarily returned characters
        std::stack<char> m_returnedChars;
};

#endif
