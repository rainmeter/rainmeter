namespace Rainmeter
{
    public class Version
    {
#if X64
        public const string Informational = "0.0.0.0 (64-bit)";
#else
        public const string Informational = "0.0.0.0 (32-bit)";
#endif
    }
}
