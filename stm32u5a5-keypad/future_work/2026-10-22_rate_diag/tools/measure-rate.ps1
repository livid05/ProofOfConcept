# Measures the real USB report rate of the "8K Rate Test" diagnostic image.
#
#   powershell -File tools\measure-rate.ps1            (5 second measurement)
#   powershell -File tools\measure-rate.ps1 -Seconds 10
#   powershell -File tools\measure-rate.ps1 -SelfTest  (checks the maths, no hardware)
#
# It opens the test device (USB ID 0483:5711), reads its numbered 8-byte reports
# for the chosen time, and prints:
#   - the rate the DEVICE sent, worked out from the device's own microsecond clock
#   - how many arrived at this program, and how many Windows dropped
#   - how far apart reports were (min / median / 99th percentile / max, microseconds)
#   - how many gaps were exactly one microframe (125 us), two (250 us), three...
#   - a verdict. 8K confirmed needs >= 7500 reports/s with >= 90% of gaps near 125 us.
# Nothing is installed or downloaded; it uses Windows' own HID and SetupAPI.
param(
    [double]$Seconds = 5,
    [string]$VidPid = 'vid_0483&pid_5711',
    [switch]$SelfTest
)
$ErrorActionPreference = 'Stop'

$source = @'
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using Microsoft.Win32.SafeHandles;

public static class RateStats
{
    // c = report counters, t = device microsecond clock, n = number of samples.
    // All differences use unchecked unsigned arithmetic so 32-bit wrap-around is harmless.
    public static string Analyze(uint[] c, uint[] t, int n, double hostSeconds, out double deviceRate, out string verdict)
    {
        deviceRate = 0;
        verdict = "NO DATA";
        if (n < 10) return "Too few reports received (" + n + ").";
        uint span = unchecked(t[n - 1] - t[0]);
        uint sentDiff = unchecked(c[n - 1] - c[0]);
        if (span == 0) return "Device clock did not advance.";
        deviceRate = sentDiff * 1000000.0 / span;
        double hostRate = n / hostSeconds;
        long dropped = (long)sentDiff + 1 - n;

        List<uint> gaps = new List<uint>();
        for (int i = 1; i < n; i++)
            if (unchecked(c[i] - c[i - 1]) == 1) gaps.Add(unchecked(t[i] - t[i - 1]));
        if (gaps.Count < 10) return "Reports were not consecutive enough to measure gaps (host dropped most of them).";
        gaps.Sort();
        uint min = gaps[0];
        uint med = gaps[gaps.Count / 2];
        uint p99 = gaps[(int)(gaps.Count * 0.99)];
        uint max = gaps[gaps.Count - 1];
        int one = 0, two = 0, three = 0, other = 0;
        foreach (uint g in gaps)
        {
            if (g >= 100 && g <= 150) one++;
            else if (g >= 225 && g <= 275) two++;
            else if (g >= 350 && g <= 400) three++;
            else other++;
        }
        double pctOne = one * 100.0 / gaps.Count;

        if (deviceRate >= 7500 && pctOne >= 90) verdict = "8K CONFIRMED";
        else if (deviceRate >= 1800) verdict = "PARTIAL (between 1K and 8K)";
        else if (deviceRate >= 900) verdict = "1K-CLASS (about 1000 reports per second)";
        else verdict = "LOW (under 900 reports per second)";

        StringBuilder sb = new StringBuilder();
        sb.AppendLine(String.Format("Device send rate (device clock): {0:F0} reports/s", deviceRate));
        sb.AppendLine(String.Format("Received by this program:       {0} reports in {1:F2} s ({2:F0}/s)", n, hostSeconds, hostRate));
        sb.AppendLine(String.Format("Dropped by Windows (not read):   {0} reports (counter gaps; does not affect the device rate)", dropped));
        sb.AppendLine(String.Format("Gap between reports (us):        min {0}   median {1}   99th pct {2}   max {3}", min, med, p99, max));
        sb.AppendLine(String.Format("Gaps near 125 us (1 microframe): {0:F1}%", pctOne));
        sb.AppendLine(String.Format("Gaps near 250 us / 375 us / other: {0:F1}% / {1:F1}% / {2:F1}%", two * 100.0 / gaps.Count, three * 100.0 / gaps.Count, other * 100.0 / gaps.Count));
        sb.AppendLine("VERDICT: " + verdict);
        return sb.ToString();
    }

    // Maths self-test with synthetic data (no hardware).
    public static string SelfTest()
    {
        string v; double r;
        // Ideal 8K, starting near the 32-bit wrap points of both the counter and the clock.
        int n = 16000;
        uint[] c = new uint[n]; uint[] t = new uint[n];
        for (int i = 0; i < n; i++) { c[i] = unchecked(0xFFFFFF00u + (uint)i); t[i] = unchecked(0xFFFFF000u + (uint)i * 125u); }
        Analyze(c, t, n, 2.0, out r, out v);
        if (v != "8K CONFIRMED" || Math.Abs(r - 8000) > 5) throw new Exception("self-test ideal 8K failed: " + v + " " + r);
        // 1K.
        n = 2000; c = new uint[n]; t = new uint[n];
        for (int i = 0; i < n; i++) { c[i] = (uint)i; t[i] = (uint)i * 1000u; }
        Analyze(c, t, n, 2.0, out r, out v);
        if (!v.StartsWith("1K-CLASS")) throw new Exception("self-test 1K failed: " + v + " " + r);
        // Mostly 125 us with every 10th gap doubled: about 7270/s, so not 8K.
        n = 16000; c = new uint[n]; t = new uint[n]; uint acc = 0;
        for (int i = 0; i < n; i++) { c[i] = (uint)i; t[i] = acc; acc += (i % 10 == 9) ? 250u : 125u; }
        Analyze(c, t, n, 2.0, out r, out v);
        if (!v.StartsWith("PARTIAL")) throw new Exception("self-test mixed failed: " + v + " " + r);
        // Windows dropping every 4th report must NOT change the device rate or verdict.
        n = 16000; c = new uint[n]; t = new uint[n]; int m = 0;
        for (int i = 0; i < n; i++) { if (i % 4 == 3) continue; c[m] = (uint)i; t[m] = (uint)i * 125u; m++; }
        Analyze(c, t, m, 2.0, out r, out v);
        if (v != "8K CONFIRMED") throw new Exception("self-test host-drop failed: " + v + " " + r);
        // Too little data.
        string s = Analyze(c, t, 3, 1.0, out r, out v);
        if (v != "NO DATA") throw new Exception("self-test no-data failed");
        return "SELF-TEST PASSED: 8K, 1K, mixed, host-drop and wrap-around cases classified correctly.";
    }
}

public static class HidReader
{
    [StructLayout(LayoutKind.Sequential)]
    struct SP_DEVICE_INTERFACE_DATA { public uint cbSize; public Guid InterfaceClassGuid; public uint Flags; public IntPtr Reserved; }

    [DllImport("hid.dll")] static extern void HidD_GetHidGuid(out Guid guid);
    [DllImport("setupapi.dll", SetLastError = true)] static extern IntPtr SetupDiGetClassDevs(ref Guid guid, IntPtr enumerator, IntPtr hwnd, uint flags);
    [DllImport("setupapi.dll", SetLastError = true)] static extern bool SetupDiEnumDeviceInterfaces(IntPtr set, IntPtr info, ref Guid guid, uint index, ref SP_DEVICE_INTERFACE_DATA data);
    [DllImport("setupapi.dll", CharSet = CharSet.Auto, SetLastError = true)] static extern bool SetupDiGetDeviceInterfaceDetail(IntPtr set, ref SP_DEVICE_INTERFACE_DATA data, IntPtr detail, uint size, out uint required, IntPtr devInfo);
    [DllImport("setupapi.dll")] static extern bool SetupDiDestroyDeviceInfoList(IntPtr set);
    [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)] static extern SafeFileHandle CreateFile(string name, uint access, uint share, IntPtr security, uint disposition, uint flags, IntPtr template);
    [DllImport("kernel32.dll", SetLastError = true)] static extern bool ReadFile(SafeFileHandle h, byte[] buffer, uint count, out uint read, IntPtr overlapped);
    [DllImport("kernel32.dll", SetLastError = true)] static extern bool CancelIoEx(SafeFileHandle h, IntPtr overlapped);

    static string FindPath(string vidPid)
    {
        Guid hid; HidD_GetHidGuid(out hid);
        IntPtr set = SetupDiGetClassDevs(ref hid, IntPtr.Zero, IntPtr.Zero, 0x12); // DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
        try
        {
            for (uint i = 0; ; i++)
            {
                SP_DEVICE_INTERFACE_DATA d = new SP_DEVICE_INTERFACE_DATA();
                d.cbSize = (uint)Marshal.SizeOf(typeof(SP_DEVICE_INTERFACE_DATA));
                if (!SetupDiEnumDeviceInterfaces(set, IntPtr.Zero, ref hid, i, ref d)) break;
                uint need; SetupDiGetDeviceInterfaceDetail(set, ref d, IntPtr.Zero, 0, out need, IntPtr.Zero);
                IntPtr buf = Marshal.AllocHGlobal((int)need);
                try
                {
                    Marshal.WriteInt32(buf, IntPtr.Size == 8 ? 8 : 6); // cbSize of the detail struct
                    if (!SetupDiGetDeviceInterfaceDetail(set, ref d, buf, need, out need, IntPtr.Zero)) continue;
                    string path = Marshal.PtrToStringAuto(new IntPtr(buf.ToInt64() + 4));
                    if (path != null && path.IndexOf(vidPid, StringComparison.OrdinalIgnoreCase) >= 0) return path;
                }
                finally { Marshal.FreeHGlobal(buf); }
            }
        }
        finally { SetupDiDestroyDeviceInfoList(set); }
        return null;
    }

    public static string Measure(string vidPid, double seconds)
    {
        string path = FindPath(vidPid);
        if (path == null)
            return "NOT FOUND: no HID device with " + vidPid + ". Flash the rate-test image, press RST with BOOT released, and replug USB.";
        SafeFileHandle h = CreateFile(path, 0x80000000u /*GENERIC_READ*/, 3 /*share read|write*/, IntPtr.Zero, 3 /*OPEN_EXISTING*/, 0, IntPtr.Zero);
        if (h.IsInvalid) return "Could not open " + path + " (Win32 error " + Marshal.GetLastWin32Error() + ").";

        List<uint> counters = new List<uint>(200000);
        List<uint> times = new List<uint>(200000);
        string error = null;
        Stopwatch sw = new Stopwatch();
        Thread reader = new Thread(delegate()
        {
            byte[] buf = new byte[9]; // 1 report-ID byte (always 0) + 8 data bytes
            uint got;
            sw.Start();
            while (sw.Elapsed.TotalSeconds < seconds)
            {
                if (!ReadFile(h, buf, 9, out got, IntPtr.Zero)) { error = "ReadFile failed (Win32 error " + Marshal.GetLastWin32Error() + ")"; break; }
                if (got != 9) { error = "Unexpected report length " + got; break; }
                counters.Add(BitConverter.ToUInt32(buf, 1));
                times.Add(BitConverter.ToUInt32(buf, 5));
            }
        });
        reader.IsBackground = true;
        reader.Start();
        if (!reader.Join((int)(seconds * 1000) + 3000))
        {
            CancelIoEx(h, IntPtr.Zero); // the device stopped sending: unblock the read
            reader.Join(1000);
            error = "Timed out: the device stopped sending reports.";
        }
        double elapsed = sw.Elapsed.TotalSeconds;
        h.Close();
        if (counters.Count < 10 && error != null) return error;
        double rate; string verdict;
        string report = RateStats.Analyze(counters.ToArray(), times.ToArray(), counters.Count, elapsed, out rate, out verdict);
        return (error != null ? "(warning: " + error + ")\r\n" : "") + report;
    }
}
'@

Add-Type -TypeDefinition $source

if ($SelfTest) {
    [RateStats]::SelfTest()
    return
}
Write-Output ("Measuring for {0} s ..." -f $Seconds)
[HidReader]::Measure($VidPid, $Seconds)
