# Data.h Prototype Fix Report

**Total fixes applied**: 66

## Changes Made:
- Changed: extern long	getmagic(void), getmagic(void), getmgkcost(void), score_one(void); -> extern long getmagic(), getmagic(), getmgkcost(), score_one();
- Changed: extern long	get_number(void), solds_in_sector(void),defaultunit(void); -> extern long get_number(), solds_in_sector(), defaultunit();
- Changed: extern int	move_file(char *from, char *to), land_2reachp(void), land_reachp(void), canbeseen(int x, int y); -> extern int move_file(char *from, char *to), land_2reachp(), land_reachp(), canbeseen(int x, int y);
- Changed: extern int	water_reachp(void), markok(void), is_habitable(void), parse(int ch); -> extern int water_reachp(), markok(), is_habitable(), parse(int ch);
- Changed: extern int	units_in_sector(int x, int y, int nation), num_powers(void), tofood(struct s_sector *sptr, int cntry), mailopen(int to); -> extern int units_in_sector(int x, int y, int nation), num_powers(), tofood(struct s_sector *sptr, int cntry), mailopen(int to);
- Changed: extern int	get_god(void), flightcost(void), todigit(void), getclass(void), startcost(void); -> extern int get_god(), flightcost(), todigit(), getclass(), startcost();
- Changed: extern int	water_2reachp(void),tg_ok(int nation, struct s_sector *sptr), readmap(void), avian(void); -> extern int water_2reachp(), tg_ok(int nation, struct s_sector *sptr), readmap(), avian();
- Changed: extern int	cbonus(void), armymove(void),takeover(void),getnewname(void); -> extern int cbonus(), armymove(), takeover(), getnewname();
- Changed: extern int getleader(void); -> extern int getleader();
- Changed: extern int	aretheyon(void),armygoto(void),navygoto(void),getselunit(void); -> extern int aretheyon(), armygoto(), navygoto(), getselunit();
- Changed: extern int	unitvalid(int type),orctake(void),fort_val(struct s_sector *sptr); -> extern int unitvalid(int type), orctake(), fort_val(struct s_sector *sptr);
- Changed: extern int	addgships(void),addmships(void),addwships(void),fltships(int country, int nvynum); -> extern int addgships(), addmships(), addwships(), fltships(int country, int nvynum);
- Changed: extern int	fltghold(void),fltwhold(int nvynum),fltmhold(int nvynum),flthold(int nvynum),compass(int x0, int y0, int x1, int y1); -> extern int fltghold(), fltwhold(int nvynum), fltmhold(int nvynum), flthold(int nvynum), compass(int x0, int y0, int x1, int y1);
- Changed: extern int	get_country(void),check_lock(char *filename, int keeplock),doclass(void),get_pass(char *str); -> extern int get_country(), check_lock(char *filename, int keeplock), doclass(), get_pass(char *str);
- Changed: extern unsigned short	fltspeed(void); -> extern unsigned short	fltspeed();
- Changed: extern void	do_pirate(void), do_nomad(void), do_savage(void), do_lizard(void); -> extern void do_pirate(), do_nomad(), do_savage(), do_lizard();
- Changed: extern void	getjewel(void),getmetal(void),loadfleet(void),removemgk(void),exenewmgk(void); -> extern void getjewel(), getmetal(), loadfleet(), removemgk(), exenewmgk();
- Changed: extern struct	s_sector *rand_sector(void); -> extern struct	s_sector *rand_sector();
- Changed: extern void	subgships(void),submships(void),subwships(void),getspace(void),sackem(void); -> extern void subgships(), submships(), subwships(), getspace(), sackem();
- Changed: extern void	whatcansee(void), reset_god(void), get_nname(char *str), camp_info(void); -> extern void whatcansee(), reset_god(), get_nname(char *str), camp_info();
- Changed: extern void	makebottom(void), check_mail(void), centermap(void); -> extern void makebottom(), check_mail(), centermap();
- Changed: extern void	checkout(void),copyscreen(void),bye(int dounlink),credits(void),init_hasseen(void); -> extern void checkout(char *__file__, int __line__), copyscreen(), bye(int dounlink), credits(), init_hasseen();
- Changed: extern void	combinearmies(void),change_status(void),reducearmy(void),splitarmy(void); -> extern void combinearmies(), change_status(), reducearmy(), splitarmy();
- Changed: extern void	errormsg(char *str), clear_bottom(int i), addgroup(void),ext_cmd(int armie); -> extern void errormsg(char *str), clear_bottom(int i), addgroup(void), ext_cmd(int armie);
- Changed: extern void	randomevent(void), wdisaster(void), weather(void), deplete(void); -> extern void randomevent(), wdisaster(), weather(), deplete();
- Changed: extern void	verify_ntn(void), verify_sct(void), verifydata(char *__file__, int __line__), prep(int nation, int leader); -> extern void verify_ntn(void), verify_sct(void), verifydata(char *__file__, int __line__), prep(int nation, int leader);
- Changed: extern void	errorbar(void), newbye(void), newreset(void), newmsg(void), newerror(void); -> extern void errorbar(), newbye(), newreset(), newmsg(), newerror();
- Changed: extern void	newinit(void), jump_to(int home); -> extern void newinit(), jump_to(int home);
- Changed: extern void	destroy(void), updmove(int race, int country), spreadsheet(void), mailclose(int to); -> extern void destroy(), updmove(int race, int country), spreadsheet(), mailclose(int to);
- Changed: extern void	updexecs(void), updcapture(void), updsectors(void); -> extern void updexecs(), updcapture(), updsectors();
- Changed: extern void	updmil(void), updcomodities(void), updleader(void); -> extern void updmil(), updcomodities(), updleader();
- Changed: extern void	nationrun(void), n_atpeace(void), n_trespass(void), n_people(void); -> extern void nationrun(), n_atpeace(), n_trespass(), n_people();
- Changed: extern void	n_toofar(void), n_unowned(void), pceattr(void), checkout(void); -> extern void n_toofar(), n_unowned(), pceattr(), checkout(char *__file__, int __line__);
- Changed: extern void	fdxyretreat(void), retreat(void), rawmaterials(void), createworld(void); -> extern void fdxyretreat(), retreat(), rawmaterials(), createworld();
- Changed: extern void	att_setup(void), att_base(void), att_bonus(void); -> extern void att_setup(), att_base(), att_bonus();
- Changed: extern void	adjarm(void),armyrpt(int repnum),atkattr(void),blowup(void); -> extern void adjarm(), armyrpt(int repnum), atkattr(), blowup();
- Changed: extern void	budget(void),change(void),cheat(void),coffmap(void),combat(void),construct(void); -> extern void budget(), change(), cheat(), coffmap(), combat(), construct();
- Changed: extern void	defattr(void),diploscrn(void),domagic(void),draft(void),erupt(void); -> extern void defattr(), diploscrn(), domagic(), draft(), erupt();
- Changed: extern void	fight(void); -> extern void	fight();
- Changed: extern void	fill_edge(void),flee(void),fleetrpt(void),hangup(int sig),help(void); -> extern void fill_edge(), flee(), fleetrpt(), hangup(int sig), help();
- Changed: extern void	highlight(int x, int y, short hmode); -> extern void highlight(int x, int y, short hmode);
- Changed: extern void	makemap(void); -> extern void	makemap();
- Changed: extern void	makeworld(void),monster(void),moveciv(void); -> extern void makeworld(), monster(), moveciv();
- Changed: extern void	mymove(void),navalcbt(void),newdip(void),newdisplay(void),newlogin(void); -> extern void mymove(), navalcbt(), newdip(), newdisplay(), newlogin();
- Changed: extern void	newspaper(void),npcredes(void),offmap(void),place(void),populate(void); -> extern void newspaper(), npcredes(), offmap(), place(), populate();
- Changed: extern void	printele(void),printnat(void),printscore(void),printveg(void); -> extern void printele(), printnat(), printscore(), printveg();
- Changed: extern void	pr_ntns(void),pr_desg(void),produce(void); -> extern void pr_ntns(), pr_desg(), produce();
- Changed: extern void	readdata(void),redesignate(void),redomil(void),reduce(void),rmessage(void),score(void); -> extern void readdata(), redesignate(), redomil(), reduce(), rmessage(), score();
- Changed: extern void	see(void),showscore(void),update(void); -> extern void see(), showscore(), update();
- Changed: extern void	wmessage(void),writedata(void),getdstatus(void); -> extern void wmessage(), writedata(), getdstatus();
- Changed: extern void	wizardry(void); -> extern void	wizardry();
- Changed: extern	char	**m2alloc(void); -> extern	char	**m2alloc();
- Fixed signature: canbeseen
- Fixed signature: mailopen
- Fixed signature: fort_val
- Fixed signature: compass
- Fixed signature: get_pass
- Fixed signature: fltships
- Fixed signature: makeside
- Fixed signature: highlight
- Fixed signature: checkout
- Fixed signature: peasant_revolt
- Fixed signature: other_revolt
- Fixed signature: prep
- Fixed signature: ext_cmd
- Fixed signature: mailclose

## Strategy Applied:
1. Restored K&R style declarations (empty parentheses) for safety
2. Fixed specific known signature mismatches
3. Preserved correct (void) declarations where appropriate

## Next Steps:
- Test compilation with these fixes
- Address any remaining compilation errors
- In Phase 8, generate proper ANSI C prototypes systematically