
# Installation of MTB PSOC Edge Tools on WSL

> [!WARNING]
> These instructions will be in the `ports/psoc-edge/README.md` file once the assets here mentioned are publicly available.
> We keep them temporarily here as the info and links cannot be shared publicly.

1. Download the [MTB tools 3.6](https://mtb-webserver.icp.infineon.com/repo/mtb_installers/ModusToolbox_3.6.0/ModusToolbox-3.6.0.17971-RC3/deploy/ModusToolbox_3.6.0.17971-linux-install.tar.gz). And untar the file in your home directory:

```
tar -xzf ModusToolbox_3.6.0.17971-linux-install.tar.gz
```

And go to the `tools_3.6/modus-shell` directory and run the postinstall script:

```
cd tools_3.6/modus-shell
./postinstall.sh
```

2. Install the compiler  [GCC ARM Embedded 14.2.1](https://mtb-webserver.icp.infineon.com/repo/gcc-standalone-package/develop/MTB-GCC-14.2.1.265-RC1/deploy/):

```
sudo apt install ./mtbgccpackage_14.2.1.265_linux_x64.deb
```
3. Install the [Edge Protect Security Suite 1.6.0](https://mtb-webserver.icp.infineon.com/repo/mtb-packs/mtb-edge-protect-security-suite/integration/pse84/478/deploy/):

```
sudo apt install ./modustoolboxedgeprotectsecuritysuite_1.6.0.478_linux_x64.deb
```

4. Install the [ModusToolbox Programming Tools 1.5.0](https://mtb-webserver.icp.infineon.com/repo/mtb-progtools/modustoolboxprogtools/release/1.5.0/1534/deploy/):

```
sudo apt install ./ModusToolboxProgtools_1.5.0.1534.deb
```

   
5. Finally, overwrite the manifest to the internal Infineon Gitlab, by setting the following environment variable:

```
export 'CyRemoteManifestOverride'=https://gitlab.intra.infineon.com/rsg /mtb-super-manifest/-/raw/psoc_edge_e84_es100/mtb-super-manifest-fv2.xml
```

You can now from the `lib/mtb-psoc-edge-libs` directory run:

```
make 
make getlibs
make build
make program
```
