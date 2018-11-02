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

            FileStream file = new FileStream(path, FileMode.Open);
            StreamReader stream = new StreamReader(file);
            string line;

            while ((line = stream.ReadLine()) != null)
            {
                string[] splitValues = line.Split(';');
                
                xValues.Add(float.Parse(splitValues[0], System.Globalization.CultureInfo.InvariantCulture));
                yValues.Add(float.Parse(splitValues[1], System.Globalization.CultureInfo.InvariantCulture));
                zValues.Add(float.Parse(splitValues[2], System.Globalization.CultureInfo.InvariantCulture));
            }
            stream.Close();

            float xMean = CalcMean(xValues);
            float yMean = CalcMean(yValues);
            float zMean = CalcMean(zValues);

            WriteLine($"Ka: x: {xMean} y: {yMean} z: {zMean}\n");

            float xVar = CalcVariance(xValues, xMean);
            float yVar = CalcVariance(yValues, yMean);
            float zVar = CalcVariance(zValues, zMean);

            WriteLine($"Varianssi: x: {xVar} y: {yVar} z: {zVar}\n");

            float[] maxZset = Get3Max(zValues);
            WriteLine($"Max z set:\n{maxZset[0]}\n{maxZset[1]}\n{maxZset[2]}\n");

            float[] minZset = Get3Min(zValues);
            WriteLine($"Min z set:\n{minZset[0]}\n{minZset[1]}\n{minZset[2]}");
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
