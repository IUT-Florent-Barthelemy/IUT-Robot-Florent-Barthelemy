﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;

//GITHUB CHECK COMMENT 03/05

namespace robotInterface_barthelemy
{
    class UART
    {
        public Queue<byte> rcvBytesQueue = new Queue<byte>();

        //sends encoded UART frames
        public void UartEncodeAndSendMessage(int msgFunction, int msgPayloadLength, byte[] msgPayload, SerialPort port)
        {
            byte[] msgToSend = new byte[msgPayloadLength + 6];

            msgToSend[0] = 0xFE;    //SOF = 0xFE
            msgToSend[1] = (byte)msgFunction;
            msgToSend[2] = (byte)(msgFunction >> 8);
            msgToSend[3] = (byte)msgPayloadLength;
            msgToSend[4] = (byte)(msgPayloadLength >> 8);

            for (int i = 0; i < msgPayloadLength; i++)  //adds payload to the msgTYoSend Bytelist from byte 5 to msgPayloadLength
                msgToSend[i + 5] = msgPayload[i];

            msgToSend[5 + msgPayloadLength] = CalculateChecksum(msgFunction, msgPayloadLength, msgPayload); //adds checkSum value to the EOF
            port.Write(msgToSend, 0, msgToSend.Length);
        }

        //calculates the UART frame checksum
        public byte CalculateChecksum(int msgFunction, int msgPayloadLength, byte[] msgPayload)
        {
            byte checksum = 0x00;

            checksum ^= (byte)(0xFE ^ (byte)msgFunction ^ (byte)(msgFunction >> 8));

            for (int i = 0; i < msgPayloadLength; i++)
                checksum ^= msgPayload[i];

            return checksum;
        }

        public enum StateReception
        {
            Waiting,
            FunctionMSB,
            FunctionLSB,
            PayloadLengthMSB,
            PayloadLengthLSB,
            Payload,
            CheckSum
        }

        StateReception rcvState = StateReception.Waiting;
        int msgDecodedFunction = 0;
        int msgDecodedPayloadLength = 0;
        byte[] msgDecodedPayload;
        int msgDecodedPayloadIndex = 0;
        byte receivedCheckSum = 0x00;

        public void DecodeMessage(byte c)
        {
            switch (rcvState)
            {
                case StateReception.Waiting:
                    if (c == 0xFE)
                        rcvState = StateReception.FunctionMSB;
                    break;

                case StateReception.FunctionMSB:
                    msgDecodedFunction = (int)(c << 8) & 0xF0;
                    rcvState = StateReception.FunctionLSB;
                    break;

                case StateReception.FunctionLSB:
                    msgDecodedFunction += c;
                    rcvState = StateReception.PayloadLengthMSB;
                    break;

                case StateReception.PayloadLengthMSB:
                    msgDecodedPayloadLength = (int)(c << 8) & 0xF0;
                    rcvState = StateReception.PayloadLengthLSB;
                    break;

                case StateReception.PayloadLengthLSB:
                    msgDecodedPayloadLength += c;

                    if (msgDecodedPayloadLength > 0)
                    {
                        msgDecodedPayloadIndex = 0;
                        msgDecodedPayload = new byte[msgDecodedPayloadLength];
                        rcvState = StateReception.Payload;
                    }
                    else
                        rcvState = StateReception.CheckSum; //if no payload, skip to CheckSum state

                    break;

                case StateReception.Payload:
                    if (msgDecodedPayloadIndex < msgDecodedPayloadLength)
                    {
                        msgDecodedPayload[msgDecodedPayloadIndex] = c;
                        msgDecodedPayloadIndex++;
                    }
                    else
                        rcvState = StateReception.CheckSum;
                    break;

                case StateReception.CheckSum:
                    receivedCheckSum = c;

                    if (CalculateChecksum(msgDecodedFunction, msgDecodedPayloadLength, msgDecodedPayload) == receivedCheckSum)
                    {
                        Console.WriteLine("NOICE!");
                    }
                    else 
                    {
                        Console.WriteLine("ERROR!");
                    }

                break;

                default:
                    rcvState = StateReception.Waiting;
                    break;
            }
        }
    }
}