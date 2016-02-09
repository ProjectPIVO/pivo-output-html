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
