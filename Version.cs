namespace Rainmeter
{
    public class Version
    {
#if X64
        public const string Informational = "3.0.0.1842 (64-bit)";
#else
        public const string Informational = "3.0.0.1842 (32-bit)";
#endif
    }
}
