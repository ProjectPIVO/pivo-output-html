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

#include "General.h"
#include "BufferedReader.h"

#include <assert.h>

BufferedReader::BufferedReader(FILE* f, int bufferLength)
{
    assert(bufferLength > 0);

    m_bufferLength = bufferLength;
    m_file = f;
    m_bufferPos = -1;

    // allocate buffer
    m_buffer = new char[bufferLength];
}

BufferedReader::~BufferedReader()
{
    delete m_buffer;
}

void BufferedReader::Close()
{
    if (m_file)
        fclose(m_file);

    m_file = nullptr;
}

bool BufferedReader::IsEof()
{
    return !m_file || feof(m_file);
}

bool BufferedReader::ReadChar(char &c)
{
    // at first, look for temporarily returned characters
    if (!m_returnedChars.empty())
    {
        // if any, return last one returned (stack, LIFO style)
        c = m_returnedChars.top();
        m_returnedChars.pop();
        return true;
    }

    // if no buffer read or we would exceed the limits, read new buffer
    if (m_bufferPos < 0 || m_bufferPos >= m_bufferLength)
    {
        // read new buffer
        if (!FillBuffer())
            return false;
    }

    // if we reached end, report it
    if (m_buffer[m_bufferPos] == '\0')
        return false;

    // return current character from buffer and move position to next
    c = m_buffer[m_bufferPos++];

    return true;
}

void BufferedReader::ReturnChar(char c)
{
    m_returnedChars.push(c);
}

bool BufferedReader::FillBuffer()
{
    if (!m_file)
        return false;

    // reset position
    m_bufferPos = 0;

    // clear buffer
    memset(m_buffer, 0, m_bufferLength);

    // read buffer from file
    if (fread(m_buffer, sizeof(char), m_bufferLength, m_file) <= 0)
        return false;

    return true;
}
