import seaborn as sns
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from argparse import ArgumentParser

# TODO: lots of repetition here (I copied it from a Jupyter Notebook). Clean up!

if __name__ == '__main__':

	# Parse args
	parser = ArgumentParser()
	parser.add_argument(
		'-d', metavar='DIR', required=True,
		help='Directory contains the outputs of cluster-gen and cluster.',
	)
	args = parser.parse_args()

	# Read data

	print('Reading values')
	with open(f'{args.d}/values') as f:
		values = f.read()
	values = list(map(float, [s for s in values.split('\n') if s]))

	print('Reading centroids_n')
	with open(f'{args.d}/centroids_n') as f:
		centroids_n = f.read()
	centroids_n = list(map(float, [s for s in centroids_n.split('\n') if s]))

	print('Reading centroids_o')
	with open(f'{args.d}/centroids_o') as f:
		centroids_o = f.read()
	centroids_o = list(map(float, [s for s in centroids_o.split('\n') if s]))

	print('Reading memberships_n')
	with open(f'{args.d}/memberships_n') as f:
		memberships_n = f.read()
	memberships_n = list(map(float, [s for s in memberships_n.split('\n') if s]))

	print('Reading memberships_o')
	with open(f'{args.d}/memberships_o') as f:
		memberships_o = f.read()
	memberships_o = list(map(float, [s for s in memberships_o.split('\n') if s]))

	# Create dataframes
	print('Creating df_n')
	df_n = pd.DataFrame({
		'x': values,
		'y': [0] * len(values),
		'membership': memberships_n,
	})

	print('Creating df_o')
	df_o = pd.DataFrame({
		'x': values,
		'y': [0] * len(values),
		'membership': memberships_o,
	})

	def savefig(path):
		print(f'Writing {path}')
		plt.savefig(path, dpi=360)

	# Plot
	print('Plotting df_n')
	fig, ax = plt.subplots(figsize=(20,2))
	sns.scatterplot(
		data=df_n, x='x', y='y', hue='membership',
		ax=ax,
		palette=sns.color_palette('bright', len(np.unique(memberships_n))),
	)
	for c in centroids_n:
		ax.axvline(x=c, linewidth=1, color='green')
	ax.get_legend().remove()
	savefig(f'{args.d}/plot_n.jpg')

	print('Plotting df_o')
	fig, ax = plt.subplots(figsize=(20,2))
	sns.scatterplot(
		data=df_o.assign(y=0), x='x', y='y', hue='membership',
		ax=ax,
		palette=sns.color_palette('bright', len(np.unique(memberships_o))),
	)
	for c in centroids_o:
		ax.axvline(x=c, linewidth=1, color='green')
	ax.get_legend().remove()
	savefig(f'{args.d}/plot_o.jpg')

