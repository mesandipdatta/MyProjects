using UnityEngine;
using System.Collections;

public class Ball : MonoBehaviour {

	public float launchSpeed;

	private Rigidbody rigidBoday;
	private AudioSource audioSource;

	public void Launch ()
	{
		rigidBoday.velocity = new Vector3 (0, 0, launchSpeed);
		audioSource.Play ();
	}

	// Use this for initialization
	void Start () {
		rigidBoday = GetComponent<Rigidbody> ();
		audioSource = GetComponent<AudioSource> ();

		Launch ();
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}
