# Source selection

Selected D1X-Rebirth 0.58.1, pinned in provenance.lock.json. Its complete C engine already supports PC shareware detection, software rendering, SDL 1.2 and PhysicsFS. A target-specific CMake integration replaces its Python-2-era SCons build for this port; original SConstruct remains available. No Vita packaging or desktop OpenGL is used. Current DXX C++23 was unnecessary once this first fallback built successfully.

Historical retrieval attempted on 2026-11-15:
- https://psp.brewology.com/downloads/get.php?id=4325 : HTTP 403
- https://store.brewology.com/dl/7828/DescentPSP_brewology_com.zip : HTTP 403 in web fetch
- https://dlhb.gamebrew.org/psphomebrew/descentforpsp.7z : HTTP 403
- https://www.dcemu.co.uk/vbulletin/threads/23572-Descent-for-PSP : redirected to unrelated YouTube page

No historical ZIP, psp-readme.txt, source or patches were recovered. Historical source availability remains unknown. No old EBOOT, patch utility or firmware dependency was run or packaged. Search ended after these bounded attempts. GameBrew describes the old release as D1X-Rebirth but that does not establish its exact upstream revision.

Engine archive SHA-256: 5befc8a2ab1159fdb4d4141ba9f0e229450592d5ada209e832101da5b0b3e38e.
Shareware archive SHA-256: 8b65eda01d03401d7a2dc71976528fbb7e0b07a93ce8ee0a9f1d9103623ae249.
Archive path inventories are recorded in docs/*.contents.json. Only regular files/directories with relative nontraversing paths were accepted. The shareware archive's DOS executables were not extracted or executed. The unmodified archive is retained locally in reference/; data hashes are in docs/edition.json.

No retail files were supplied. Discovery was limited to this project directory; no disk/account scan occurred. The authorized DXX-linked shareware archive was used. Networking remains compiled engine infrastructure without a transport; no ad-hoc/IPX/infrastructure functionality is claimed.
