{
  "targets": [
      ],
  "conditions": [
    ['OS=="win"', {
        "targets": [{
            "target_name": "editorVersion",
            "include_dirs" : [
              "src",
              "<!(node -e \"require('nan')\")"
            ],
            "sources": [
              "fetchUnityVersion.cc"
            ],
            "libraries": [ "Version.lib" ]
          }]
    }],
  ],
}
