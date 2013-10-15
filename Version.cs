namespace Rainmeter
{
    public class Version
    {
#if X64
        public const string Informational = "3.0.1.2121 (64-bit)";
#else
        public const string Informational = "3.0.1.2121 (32-bit)";
#endif
    }
}
