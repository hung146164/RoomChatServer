#pragma once
#include "Net_Common.h"

namespace olc
{
    namespace net
    {
		template<typename T>
		struct message_header
		{
			T id{}; //{} is the default value of T;
			uint32_t size=0;
		};
		
		template<typename T>
		struct message
		{
			message_header<T> header{};
			std::vector<uint8_t> body;
			uint32_t size() const 
			{
				return body.size();
			}
			
			template<typename DataType>
			friend message<T>& operator <<(message<T> & msg, const DataType& data)
			{
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex");
				
				uint32_t currentIndex=msg.body.size();
				uint32_t newSize= msg.header.size+sizeof(DataType);

				msg.body.resize(newSize);
				memcpy(msg.body.data()+currentIndex,&data,sizeof(DataType));
				msg.header.size=newSize;
				return msg;
			}
			template<typename DataType>
			friend message<T>& operator >>(message<T>& msg, DataType& data)
			{
				if(msg.body.size()<sizeof(DataType)) 
				{
					throw std::runtime_error("Message body underflow");
				}
				uint32_t newSize= msg.body.size() - sizeof(DataType);
				
				memcpy(&data,msg.body.data()+newSize,sizeof(DataType));

				msg.body.resize(newSize);

				msg.header.size=newSize;

				return msg;
			}
		};

		template <typename T>
		class connection;

		template <typename T>
		struct owned_message
		{
			std::shared_ptr<connection<T>> remote = nullptr;
			message<T> msg;

			friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg)
			{
				os << msg.msg;
				return os;
			}
		};		
    }
}