using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using static System.Console;

namespace kuntoCalc
{
    class Program
    {
        // Ota huomioon että sulla pitää olla joku ohjelma jolla buildaat tän.
        // ite käytin visual studion 2017 community versiota (suosittelen),
        // mut jos et meinaa käyttää sitä mihinkää ni aika iso asennus kyllä vaa tätä varten.

        static void Main(string[] args)
        {
            string path;
            if (args.Length == 0)
            {
                WriteLine("Anna tiedostonimi.");
                return;
            }
            else
            {
                path = args[0].ToString();
            }

            CalcSingleFile(path);
        }

        private static void CalcSingleFile(string path)
        {
            List<float> xValues = new List<float>();
            List<float> yValues = new List<float>();
            List<float> zValues = new List<float>();
            List<float> presValues = new List<float>();

            FileStream file = new FileStream(path, FileMode.Open);
            StreamReader stream = new StreamReader(file);
            string line;

            while ((line = stream.ReadLine()) != null)
            {
                string[] splitValues = line.Split(';');
                
                xValues.Add(float.Parse(splitValues[0], System.Globalization.CultureInfo.InvariantCulture));
                yValues.Add(float.Parse(splitValues[1], System.Globalization.CultureInfo.InvariantCulture));
                zValues.Add(float.Parse(splitValues[2], System.Globalization.CultureInfo.InvariantCulture));
                presValues.Add(float.Parse(splitValues[3], System.Globalization.CultureInfo.InvariantCulture));
            }
            stream.Close();

            float xMean = CalcMean(xValues);
            float yMean = CalcMean(yValues);
            float zMean = CalcMean(zValues);

            //WriteLine($"Ka: x: {xMean} y: {yMean} z: {zMean}\n");

            float xVar = CalcVariance(xValues, xMean);
            float yVar = CalcVariance(yValues, yMean);
            float zVar = CalcVariance(zValues, zMean);

            //WriteLine($"Varianssi: x: {xVar} y: {yVar} z: {zVar}\n");

            float[] maxZset = Get3Max(zValues);
            //WriteLine($"Max z set:\n{maxZset[0]}\n{maxZset[1]}\n{maxZset[2]}\n");

            float[] minZset = Get3Min(zValues);
            //WriteLine($"Min z set:\n{minZset[0]}\n{minZset[1]}\n{minZset[2]}\n");

            float[] arx = new float[10];
            float[] ary = new float[10];
            float[] arz = new float[10];
            float[] pres = new float[10];
            float xAvg, yAvg, zAvg, zVari, presAvg, lastPres = 0, dif = 0;

            int c = 0, i = 0, j = 0, a = 0;


            for (a = 0; a < 20; a++)
            {
                for (i = 0; i < 10; i++)
                {
                    arx[i] = xValues.ElementAt(j);
                    ary[i] = yValues.ElementAt(j);
                    arz[i] = zValues.ElementAt(j);
                    pres[i] = presValues.ElementAt(j);
                    j++;
                }

                xAvg = CalcAvg(arx);
                yAvg = CalcAvg(ary);
                zAvg = CalcAvg(arz);
                presAvg = CalcAvg(pres);
                if (a == 0)
                {
                    lastPres = presAvg;
                }
                dif = presAvg - lastPres;
                zVari = CalcVar(arz, zAvg);
                WriteLine($"x:{xAvg} y:{yAvg} z:{zAvg} z var:{zVari} last pres:{lastPres} pres:{presAvg} dif:{dif}");
                lastPres = presAvg;


                //Lift Up
                if ((0.0016 <= zVari && zVari <= 0.0043 && dif < 0) || (0.00025 <= zVari && zVari <= 0.0043 && dif < -0.07 && dif > -0.9))
                {
                    WriteLine($"Lift Up");
                }
                //Lift Down
                else if ((0.0015 <= zVari && zVari <= 0.0043 && dif > 0) || (0.00025 <= zVari && zVari <= 0.0043 && 0.051 < dif && dif < 0.9))
                {
                    WriteLine($"Lift Down");
                }
                //Stairs Up
                else if ((0.05 < zVari || (0.09 < xAvg || 0.08 < yAvg)) && dif < 0)
                {
                    WriteLine($"Stairs Up");
                }
                //Stairs Down
                else if ((0.05 < zVari || (0.09 < xAvg || 0.08 < yAvg)) && dif > 0)
                {
                    WriteLine($"Stairs Down");
                }
                //Idle
                else
                {
                    WriteLine($"Idle");
                }
            }
        }

        private static float CalcAvg(float[] data)
        {
            int i = 0;
            float sum = 0;
            for (i = 0; i < 10; i++)
            {
                sum += data[i];
            }
            return sum / 10;
        }

        private static float CalcVar(float[] data, float avg)
        {
            int i = 0;
            float sum = 0;
            for (i = 0; i < 10; i++)
            {
                sum += (data[i] - avg) * (data[i] - avg);
            }
            return sum / 9;
        }

        private static float CalcMean(List<float> data)
        {
            float sum = 0;
            foreach (float d in data)
            {
                sum += d;
            }

            return sum / data.Count();
        }

        private static float[] Get3Max(List<float> data)
        {
            float[] maxSet = new float[3];

            for (int i = 0; i < 3; i++)
            {
                float max = data.Max();
                data.Remove(max);
                maxSet[i] = max;
            }

            return maxSet;
        }

        private static float[] Get3Min(List<float> data)
        {
            float[] minSet = new float[3];

            for (int i = 0; i < 3; i++)
            {
                float min = data.Min();
                data.Remove(min);
                minSet[i] = min;
            }

            return minSet;
        }

        private static float CalcVariance(List<float> data, float mean)
        {
            float sum = 0;

            foreach (float d in data)
            {
                sum += (d - mean)*(d - mean);
            }

            return sum / (data.Count - 1);
        }
    }
}
