using System;

namespace SevenZip.Compression.RangeCoder
{
	class Encoder
	{
		const int kNumTopBits = 24;
		public const uint kTopValue = (1 << kNumTopBits);

		System.IO.Stream Stream;
		// Buffer.OutBuffer Stream = new Buffer.OutBuffer(1 << 20);

		public UInt64 Low;
		public uint Range;
		uint _ffNum;
		byte _cache;

		long StartPosition;

		public void SetStream(System.IO.Stream stream)
		{
			Stream = stream;
			// Stream.SetStream(stream);
		}

		public void ReleaseStream()
		{
			Stream = null;
			// Stream.ReleaseStream();
		}

		public void Init()
		{
			// Stream.Init();
			StartPosition = Stream.Position;

			Low = 0;
			Range = 0xFFFFFFFF;
			_ffNum = 0;
			_cache = 0;
		}

		public void FlushData()
		{
			for (int i = 0; i < 5; i++)
				ShiftLow();
			// Stream.FlushData();
		}

		public void FlushStream()
		{
			Stream.Flush();
			// Stream.FlushStream();
		}

		public void CloseStream()
		{
			Stream.Close();
			// Stream.CloseStream();
		}

		public void Encode(uint start, uint size, uint total)
		{
			Low += start * (Range /= total);
			Range *= size;
			while (Range < kTopValue)
			{
				Range <<= 8;
				ShiftLow();
			}
		}

		public void ShiftLow()
		{
			if (Low < (uint)0xFF000000 || (uint)(Low >> 32) == 1)
			{
				Stream.WriteByte((byte)(_cache + (Low >> 32)));
				for (; _ffNum != 0; _ffNum--)
					Stream.WriteByte((byte)(0xFF + (Low >> 32)));
				_cache = (byte)(((uint)Low) >> 24);
			}
			else
				_ffNum++;
			Low = ((uint)Low) << 8;
		}

		public void EncodeDirectBits(uint v, int numTotalBits)
		{
			for (int i = numTotalBits - 1; i >= 0; i--)
			{
				Range >>= 1;
				if (((v >> i) & 1) == 1)
					Low += Range;
				if (Range < kTopValue)
				{
					Range <<= 8;
					ShiftLow();
				}
			}
		}

		public void EncodeBit(uint size0, int numTotalBits, uint symbol)
		{
			uint newBound = (Range >> numTotalBits) * size0;
			if (symbol == 0)
				Range = newBound;
			else
			{
				Low += newBound;
				Range -= newBound;
			}
			while (Range < kTopValue)
			{
				Range <<= 8;
				ShiftLow();
			}
		}

		public long GetProcessedSizeAdd()
		{
			return _ffNum +
				Stream.Position - StartPosition;
			// (long)Stream.GetProcessedSize();
		}
	}

	class Decoder
	{
		const int kNumTopBits = 24;
		public const uint kTopValue = (1 << kNumTopBits);
		public uint Range;
		public uint Code;
		// public Buffer.InBuffer Stream = new Buffer.InBuffer(1 << 16);
		public System.IO.Stream Stream;

		public void Init(System.IO.Stream stream)
		{
			// Stream.Init(stream);
			Stream = stream;

			Code = 0;
			Range = 0xFFFFFFFF;
			for (int i = 0; i < 5; i++)
				Code = (Code << 8) | (byte)Stream.ReadByte();
		}

		public void ReleaseStream()
		{
			// Stream.ReleaseStream();
			Stream = null;
		}

		public void CloseStream()
		{
			Stream.Close();
		}

		public void Normalize()
		{
			while (Range < kTopValue)
			{
				Code = (Code << 8) | (byte)Stream.ReadByte();
				Range <<= 8;
			}
		}

		public void Normalize2()
		{
			if (Range < kTopValue)
			{
				Code = (Code << 8) | (byte)Stream.ReadByte();
				Range <<= 8;
			}
		}

		public uint GetThreshold(uint total)
		{
			return Code / (Range /= total);
		}

		public void Decode(uint start, uint size, uint total)
		{
			Code -= start * Range;
			Range *= size;
			Normalize();
		}

		public uint DecodeDirectBits(int numTotalBits)
		{
			uint range = Range;
			uint code = Code;
			uint result = 0;
			for (int i = numTotalBits; i > 0; i--)
			{
				range >>= 1;
				/*
				result <<= 1;
				if (code >= range)
				{
					code -= range;
					result |= 1;
				}
				*/
				uint t = (code - range) >> 31;
				code -= range & (t - 1);
				result = (result << 1) | (1 - t);

				if (range < kTopValue)
				{
					code = (code << 8) | (byte)Stream.ReadByte();
					range <<= 8;
				}
			}
			Range = range;
			Code = code;
			return result;
		}

		public uint DecodeBit(uint size0, int numTotalBits)
		{
			uint newBound = (Range >> numTotalBits) * size0;
			uint symbol;
			if (Code < newBound)
			{
				symbol = 0;
				Range = newBound;
			}
			else
			{
				symbol = 1;
				Code -= newBound;
				Range -= newBound;
			}
			Normalize();
			return symbol;
		}

		// ulong GetProcessedSize() {return Stream.GetProcessedSize(); }
	}
}
