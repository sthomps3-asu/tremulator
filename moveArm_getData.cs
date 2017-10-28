using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Linq;
using System.IO;

public class moveArm : MonoBehaviour {

	private List<double> shoulder_data_W = new List<double>();
	private List<double> shoulder_data_X = new List<double>();
	private List<double> shoulder_data_Y = new List<double>();
	private List<double> shoulder_data_Z = new List<double>();
	private List<double> elbow_data_W = new List<double>();
	private List<double> elbow_data_X = new List<double>();
	private List<double> elbow_data_Y = new List<double>();
	private List<double> elbow_data_Z = new List<double>();

	private int i = 0;
	public GameObject elbow;

	public string shoulderCsv = "/Users/stephenthompson/Desktop/Spring2017/EEE488/repo/tremulator/unity/Tremulator/Data/imudata_bus0_elbow_static_holds.csv";
	public string elbowCsv = "/Users/stephenthompson/Desktop/Spring2017/EEE488/repo/tremulator/unity/Tremulator/Data/imudata_bus1_wrist_static_holds.csv";

	Quaternion nextElbowQuat = Quaternion.Euler (0, 0, 0);
	Quaternion nextShoulderQuat = Quaternion.Euler (0, 0, 0);
	Quaternion rotElbow = Quaternion.Euler(0, 0, 0);
	Quaternion rotShoulder = Quaternion.Euler(0, 0, 0);

	// Quaternion Data Column Headings
	public const string	QUATERNION_DATA_W = "QUATERNION_DATA_W";
	public const string	QUATERNION_DATA_X = "QUATERNION_DATA_X";
	public const string	QUATERNION_DATA_Y = "QUATERNION_DATA_Y";
	public const string	QUATERNION_DATA_Z = "QUATERNION_DATA_Z";

	struct column
	{
		public string name;
		public int offset;
		public column(string n, int o)
		{
			name = n;
			offset = o;
		}
	};

	List<float> convertQuaternions(List<column> columns, ushort[] row)
	{
		List<float> normalizedRow = new List<float> ();
		float sumOfSquares = 0;
		float magnitude;
		List<int> offsets = new List<int> ();
		List<float> tempQuats = new List<float> ();

		// Get the offsets for each coordinate
		for (int idx = 0; idx < columns.Count; idx++)
		{
			if (columns[idx].name.Contains(QUATERNION_DATA_W) || columns[idx].name.Contains(QUATERNION_DATA_X) 
				|| columns[idx].name.Contains(QUATERNION_DATA_Y) || columns[idx].name.Contains(QUATERNION_DATA_Z))
			{
				offsets.Add (columns[idx].offset - 1);
			}
		}
			
		// Scale each quaternion appropriately
		for (int idx = 0; idx < offsets.Count; idx++)
		{
			float quat = (float)row [offsets[idx]];
			if (quat >= System.Math.Pow (2, 15))
			{	
				quat = (float)(quat - System.Math.Pow(2,16));

			}
				
			tempQuats.Add((float)(quat / (System.Math.Pow(2,15) - 1)));

		}

		// Compute the sum of squares from W,X,Y,Z
		foreach (float data in tempQuats)
		{
			sumOfSquares += (float)System.Math.Pow(data, 2);
		}
			
		// Compute the magnitude of the 4 quats
		magnitude = (float)System.Math.Sqrt (sumOfSquares);

		// Normalize each quaternion
		for (int idx = 0; idx < offsets.Count; idx++)
		{
			normalizedRow.Add (tempQuats[idx] / magnitude);
		}

		return normalizedRow;
	}

	void getQuaternionData(string csvFile, List<double> quat_data_W, List<double> quat_data_X, List<double> quat_data_Y, List<double> quat_data_Z)
	{
		// Read in lines
		string[] csvLines = File.ReadAllLines(csvFile, System.Text.Encoding.UTF8);


		// Get columns
		string[] rawColumns = csvLines[0].Split(',');
		List<column> columns = new List<column> ();

		// Create column structs with their string heading and corresponding indeces
		for (int idx = 0; idx < rawColumns.Length; idx++)
		{
			columns.Add (new column (rawColumns [idx], idx));
		}

		// Grab the data for each coordinate and normalize
		for (int idx = 1; idx < csvLines.Length; idx++)
		{
			ushort[] row = csvLines [idx].Split (',').Skip (1).Select (x => System.Convert.ToUInt16 (x)).ToArray ();

			// Scale and normalize raw quaternion data
			List<float> normalizedRow = convertQuaternions (columns, row);

			// Add data to quaternion lists
			quat_data_W.Add(normalizedRow[0]);
			quat_data_X.Add(normalizedRow[1]);
			quat_data_Y.Add(normalizedRow[2]);
			quat_data_Z.Add(normalizedRow[3]);
		}
	}

	// Use this for initialization
	void Start ()
	{
		// Get Quaternion Data For Elbow and Shoulder
		getQuaternionData(elbowCsv, elbow_data_W, elbow_data_X, elbow_data_Y, elbow_data_Z);
		getQuaternionData(shoulderCsv, shoulder_data_W, shoulder_data_X, shoulder_data_Y, shoulder_data_Z);

		transform.rotation = Quaternion.Euler(0, 0, 0);
	}

	// Update is called once per frame
	void Update () 
	{
		if(i < shoulder_data_Z.Count)
		{
			nextShoulderQuat.Set((float)shoulder_data_X[i], (float)shoulder_data_Y[i], (float)shoulder_data_Z[i], (float)shoulder_data_W[i]);
			nextElbowQuat.Set((float)elbow_data_X[i], (float)elbow_data_Y[i], (float)elbow_data_Z[i], (float)elbow_data_W[i]);
			rotElbow = Quaternion.Inverse (nextShoulderQuat)*nextElbowQuat;
			transform.rotation = Quaternion.Slerp(transform.rotation, nextShoulderQuat, Time.time * (float)0.1);
			elbow.transform.localRotation = Quaternion.Slerp (elbow.transform.localRotation, rotElbow, (float)Time.time * (float)0.1);
			elbow.transform.localRotation = Quaternion.Slerp (elbow.transform.localRotation, rotElbow, (float)Time.time * (float)0.1);
			/*
			if (transform.rotation == nextShoulderQuat && elbow.transform.localRotation == rotElbow){
				i += 1;
			}
			*/

			i += 1;
		}

		//else transform.rotation = Quaternion.Slerp (transform.rotation, Quaternion.Euler (0, 0, 0), Time.deltaTime * 100);
		//Debug.Log (transform.rotation);
	}
}
